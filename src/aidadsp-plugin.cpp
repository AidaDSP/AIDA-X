/*
 * Aida-X DPF plugin
 * Copyright (C) 2022-2023 Massimo Pennazio <maxipenna@libero.it>
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "DistrhoPlugin.hpp"

#include "Biquad.h"
#include "ExpSmoother.hpp"
#include "Files.hpp"

#include "model_variant.hpp"
#include "extra/ScopedDenormalDisable.hpp"
#include "extra/Sleep.hpp"

#include <atomic>
#include <strstream>

#include "dr_flac.h"
#include "dr_wav.h"
// -Wunused-variable
#include "CDSPResampler.h"

#include "TwoStageThreadedConvolver.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

/* Define a constexpr for converting a gain in dB to a coefficient */
static constexpr float DB_CO(const float g) { return g > -90.f ? std::pow(10.f, g * 0.05f) : 0.f; }

/* Define a constexpr to scale % to coeff */
static constexpr float PC_CO(const float g) { return g < 100.f ? (g / 100.f) : 1.f; }

/* Define a macro to re-maps a number from one range to another  */
static constexpr float MAP(const float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
    return ((x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min;
}

/* Defines for tone controls */
static constexpr const float COMMON_Q = 0.707f;
static constexpr const float DEPTH_FREQ = 75.f;
static constexpr const float PRESENCE_FREQ = 900.f;

/* Defines for antialiasing filter */
static constexpr const float INLPF_MAX_CO = 0.99f * 0.5f; /* coeff * ((samplerate / 2) / samplerate) */
static constexpr const float INLPF_MIN_CO = 0.25f * 0.5f; /* coeff * ((samplerate / 2) / samplerate) */

/* Gain compensation for cabinet IR (-6dB) */
static constexpr const float kCabinetMaxGain = 0.5f;

// --------------------------------------------------------------------------------------------------------------------

struct AidaToneControl {
    Biquad dc_blocker { bq_type_highpass, 0.5f, COMMON_Q, 0.0f };
    Biquad in_lpf { bq_type_lowpass, 0.5f, COMMON_Q, 0.0f };
    Biquad bass { bq_type_lowshelf, 0.5f, COMMON_Q, 0.0f };
    Biquad mid { bq_type_peak, 0.5f, COMMON_Q, 0.0f };
    Biquad treble { bq_type_highshelf, 0.5f, COMMON_Q, 0.0f };
    Biquad depth { bq_type_highshelf, 0.5f, COMMON_Q, 0.0f };
    Biquad presence { bq_type_highshelf, 0.5f, COMMON_Q, 0.0f };
    ExpSmoother pregain;
    ExpSmoother mastergain;
    bool net_bypass = false;
    bool eq_bypass = false;
    EqPos eq_pos = kEqPost;
    MidEqType mid_type = kMidEqPeak;

    AidaToneControl()
    {
        pregain.setTimeConstant(1);
        mastergain.setTimeConstant(1);
    }

    void setSampleRate(const float parameters[kNumParameters], const double sampleRate)
    {
        dc_blocker.setFc(35.0f / sampleRate);

        in_lpf.setFc(MAP(parameters[kParameterINLPF], 0.0f, 100.0f, INLPF_MAX_CO, INLPF_MIN_CO));

        bass.setBiquad(bq_type_lowshelf,
                       parameters[kParameterBASSFREQ] / sampleRate, COMMON_Q, parameters[kParameterBASSGAIN]);

        mid.setBiquad(mid_type == kMidEqBandpass ? bq_type_bandpass : bq_type_peak,
                      parameters[kParameterMIDFREQ] / sampleRate,
                      parameters[kParameterMIDQ],
                      parameters[kParameterMIDGAIN]);

        treble.setBiquad(bq_type_highshelf,
                         parameters[kParameterTREBLEFREQ] / sampleRate, COMMON_Q, parameters[kParameterTREBLEGAIN]);

        depth.setBiquad(bq_type_highshelf,
                        DEPTH_FREQ / sampleRate, COMMON_Q, parameters[kParameterDEPTH]);

        presence.setBiquad(bq_type_highshelf,
                           PRESENCE_FREQ / sampleRate, COMMON_Q, parameters[kParameterPRESENCE]);

        pregain.setSampleRate(sampleRate);
        pregain.setTarget(DB_CO(parameters[kParameterPREGAIN]));

        mastergain.setSampleRate(sampleRate);
        mastergain.setTarget(DB_CO(parameters[kParameterMASTER]));
    }
};

#if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
struct AudioFile {
    float* buffer;
    drwav_uint64 currentFrame;
    drwav_uint64 numFrames;
    bool resampled;

    AudioFile(float* const buffer_, const drwav_uint64 numFrames_, const bool resampled_) noexcept
        : buffer(buffer_),
          currentFrame(0),
          numFrames(numFrames_),
          resampled(resampled_) {}

    ~AudioFile() noexcept
    {
        if (resampled)
            delete[] buffer;
        else
            free(buffer);
    }
};
#endif

struct DynamicModel {
    ModelVariantType variant;
    bool input_skip; /* Means the model has been trained with first input element skipped to the output */
    float input_gain;
    float output_gain;
};

// --------------------------------------------------------------------------------------------------------------------
// Apply a gain ramp to a buffer

static void applyGainRamp(ExpSmoother& smoother, float* const out, const uint32_t numSamples)
{
    for (uint32_t i=0; i<numSamples; ++i)
        out[i] *= smoother.next();
}

// --------------------------------------------------------------------------------------------------------------------
// Apply filter

static void applyBiquadFilter(Biquad& filter, float* const out, const float* const in, const uint32_t numSamples)
{
    for (uint32_t i=0; i<numSamples; ++i)
        out[i] = filter.process(in[i]);
}

static void applyBiquadFilter(Biquad& filter, float* const out, const uint32_t numSamples)
{
    for (uint32_t i=0; i<numSamples; ++i)
        out[i] = filter.process(out[i]);
}

// --------------------------------------------------------------------------------------------------------------------
// Apply biquad cascade filters

static void applyToneControls(AidaToneControl& aida, float* const out, uint32_t numSamples)
{
    if (aida.mid_type == kMidEqBandpass)
    {
        applyBiquadFilter(aida.mid, out, numSamples);
    }
    else
    {
        applyBiquadFilter(aida.depth, out, numSamples);
        applyBiquadFilter(aida.bass, out, numSamples);
        applyBiquadFilter(aida.mid, out, numSamples);
        applyBiquadFilter(aida.treble, out, numSamples);
        applyBiquadFilter(aida.presence, out, numSamples);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// This function carries model calculations for snapshot models

void applyModel(DynamicModel* model, float* const out, uint32_t numSamples)
{
    const bool input_skip = model->input_skip;
    const float input_gain = model->input_gain;
    const float output_gain = model->output_gain;

    std::visit(
        [&input_skip, &out, numSamples, input_gain, output_gain] (auto&& custom_model)
        {
            using ModelType = std::decay_t<decltype (custom_model)>;
            if constexpr (ModelType::input_size == 1)
            {
                if (input_skip)
                {
                    for (uint32_t i=0; i<numSamples; ++i)
                    {
                        out[i] *= input_gain;
                        out[i] += custom_model.forward(out + i) * output_gain;
                    }
                }
                else
                {
                    for (uint32_t i=0; i<numSamples; ++i)
                    {
                        out[i] *= input_gain;
                        out[i] = custom_model.forward(out + i) * output_gain;
                    }
                }
            }
            else
            {
                // TODO
            }
        },
        model->variant
    );
}

// --------------------------------------------------------------------------------------------------------------------

class AidaDSPLoaderPlugin : public Plugin
{
    AidaToneControl aida;
    DynamicModel* model = nullptr;
    TwoStageThreadedConvolver* cabsim = nullptr;
    std::atomic<bool> activeModel { false };
    std::atomic<bool> activeConvolver { false };
    String cabsimFilename;
    ExpSmoother cabsimGain;
    float* cabsimInplaceBuffer = nullptr;
    ExpSmoother bypassGain;
    float* bypassInplaceBuffer = nullptr;
    float parameters[kNumParameters];
    std::atomic<bool> resetMeterIn { false };
    std::atomic<bool> resetMeterOut { false };
   #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
    AudioFile* audiofile = nullptr;
    std::atomic<bool> activeAudiofile { false };
   #endif

public:
    AidaDSPLoaderPlugin()
        : Plugin(kNumParameters, 0, kStateCount) // parameters, programs, states
    {
        // Initialize parameters to their defaults
        for (uint i=0; i<kNumParameters; ++i)
            parameters[i] = kParameters[i].ranges.def;

        bypassGain.setTimeConstant(1);
        bypassGain.setTarget(1.f);

        cabsimGain.setTimeConstant(1);
        cabsimGain.setTarget(kCabinetMaxGain);

        // initialize
        bufferSizeChanged(getBufferSize());
        sampleRateChanged(getSampleRate());

        // load default model
        loadDefaultModel();

       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
        // and audio file
        loadAudioFile(kAudioLoopFilenames[0]);
       #endif
    }

    ~AidaDSPLoaderPlugin()
    {
        delete model;
        delete cabsim;
       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
        delete audiofile;
       #endif
        delete[] bypassInplaceBuffer;
        delete[] cabsimInplaceBuffer;
    }

protected:
   /* -----------------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      A plugin label follows the same rules as Parameter::symbol, with the exception that it can start with numbers.
    */
    const char* getLabel() const override
    {
        return "AIDA_X";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Simple loader for neural models using RTNeural inference engine.";
    }

   /**
      Get the plugin author/maker.
    */
    const char* getMaker() const override
    {
        return DISTRHO_PLUGIN_BRAND;
    }

   /**
      Get the plugin homepage.
    */
    const char* getHomePage() const override
    {
        return "https://aidadsp.github.io/";
    }

   /**
      Get the plugin license name (a single line of text).
      For commercial plugins this should return some short copyright information.
    */
    const char* getLicense() const override
    {
        return "GPL-3.0-or-later";
    }

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const override
    {
        return kVersionNumber;
    }

   /**
      Get the plugin unique Id.
      This value is used by LADSPA, DSSI and VST plugin formats.
    */
    int64_t getUniqueId() const override
    {
        return d_cconst('a', 'i', 'd', 'a');
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the audio port @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    void initAudioPort(const bool input, const uint32_t index, AudioPort& port) override
    {
        // treat meter audio ports as mono
        port.groupId = kPortGroupMono;

        // everything else is as default
        Plugin::initAudioPort(input, index, port);
    }

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(const uint32_t index, Parameter& parameter) override
    {
        parameter = kParameters[index];

        if (index == kParameterGLOBALBYPASS)
            parameter.designation = kParameterDesignationBypass;
    }

   /**
      Initialize the state @a index.@n
      This function will be called once, shortly after the plugin is created.@n
    */
    void initState(const uint32_t index, State& state) override
    {
        switch (static_cast<States>(index))
        {
        case kStateModelFile:
            state.hints = kStateIsFilenamePath;
            state.key = "json";
            state.defaultValue = "default";
            state.label = "Neural Model";
            state.description = "";
           #ifdef __MOD_DEVICES__
            state.fileTypes = "aidadspmodel";
           #endif
            break;
        case kStateImpulseFile:
            state.hints = kStateIsFilenamePath;
            state.key = "cabinet";
            state.defaultValue = "default";
            state.label = "Cabinet Impulse Response";
            state.description = "";
           #ifdef __MOD_DEVICES__
            state.fileTypes = "cabsim";
           #endif
            break;
       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
        case kStateAudioFile:
            state.hints = kStateIsFilenamePath;
            state.key = "audiofile";
            state.defaultValue = "";
            state.label = "Audio Loop File";
            state.description = "";
           #ifdef __MOD_DEVICES__
            state.fileTypes = "audioloop";
           #endif
            break;
       #endif
        case kStateCount:
            break;
        }
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
    */
    float getParameterValue(const uint32_t index) const override
    {
        return parameters[index];
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(const uint32_t index, const float value) override
    {
        parameters[index] = value;

        const double sampleRate = getSampleRate();

        switch (static_cast<Parameters>(index))
        {
        case kParameterINLPF:
            aida.in_lpf.setFc(MAP(value, 0.0f, 100.0f, INLPF_MAX_CO, INLPF_MIN_CO));
            break;
        case kParameterPREGAIN:
            aida.pregain.setTarget(DB_CO(value));
            break;
        case kParameterNETBYPASS:
            aida.net_bypass = value > 0.5f;
            break;
        case kParameterEQBYPASS:
            aida.eq_bypass = value > 0.5f;
            break;
        case kParameterEQPOS:
            aida.eq_pos = value > 0.5f ? kEqPre : kEqPost;
            break;
        case kParameterBASSGAIN:
            aida.bass.setPeakGain(value);
            break;
        case kParameterBASSFREQ:
            aida.bass.setFc(value / sampleRate);
            break;
        case kParameterMIDGAIN:
            aida.mid.setPeakGain(value);
            break;
        case kParameterMIDFREQ:
            aida.mid.setFc(value / sampleRate);
            break;
        case kParameterMIDQ:
            aida.mid.setQ(value);
            break;
        case kParameterMTYPE:
            aida.mid_type = value > 0.5f ? kMidEqBandpass : kMidEqPeak;
            break;
        case kParameterTREBLEGAIN:
            aida.treble.setPeakGain(value);
            break;
        case kParameterTREBLEFREQ:
            aida.treble.setFc(value / sampleRate);
            break;
        case kParameterDEPTH:
            aida.depth.setPeakGain(value);
            break;
        case kParameterPRESENCE:
            aida.presence.setPeakGain(value);
            break;
        case kParameterMASTER:
            aida.mastergain.setTarget(DB_CO(value));
            break;
        case kParameterCABSIMBYPASS:
            cabsimGain.setTarget(value > 0.5f ? 0.f : kCabinetMaxGain);
            break;
        case kParameterGLOBALBYPASS:
            bypassGain.setTarget(value > 0.5f ? 0.f : 1.f);
            break;
        case kParameterMeterIn:
        case kParameterMeterOut:
        case kParameterCount:
            break;
        }
    }

    void setState(const char* const key, const char* const value) override
    {
        if (std::strncmp(key, "reset-meter-", 12) == 0)
        {
            if (key[12] == 'i')
                resetMeterIn.store(true);
            else
                resetMeterOut.store(true);
            return;
        }

        const bool isDefault = value == nullptr || value[0] == '\0' || std::strcmp(value, "default") == 0;

        if (std::strcmp(key, "json") == 0)
            return isDefault ? loadDefaultModel() : loadModelFromFile(value);
        if (std::strcmp(key, "cabinet") == 0)
            return isDefault ? loadDefaultCabinet() : loadCabinetFromFile(value);
       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
        if (std::strcmp(key, "audiofile") == 0)
            return loadAudioFile(value);
       #endif
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Model loader */

    void loadDefaultModel()
    {
        using namespace Files;

        try {
            std::istrstream jsonStream(static_cast<const char*>(static_cast<const void*>(US_Double_Nrm_ModelData)),
                                       US_Double_Nrm_ModelDataSize);
            loadModelFromStream(jsonStream);
        }
        catch (const std::exception& e) {
            d_stderr2("Unable to load json, error: %s", e.what());
        };
    }

    void loadModelFromFile(const char* const filename)
    {
        try {
            std::ifstream jsonStream(filename, std::ifstream::binary);
            loadModelFromStream(jsonStream);
        }
        catch (const std::exception& e) {
            d_stderr2("Unable to load json file: %s\nError: %s", filename, e.what());
        };
    }

    void loadModelFromStream(std::istream& jsonStream)
    {
        int input_skip;
        float input_gain;
        float output_gain;
        nlohmann::json model_json;

        try {
            jsonStream >> model_json;

            /* Understand which model type to load */
            if(model_json["in_shape"].back().get<int>() > 1) {
                throw std::invalid_argument("Values for input_size > 1 are not supported");
            }

            if (model_json["in_skip"].is_number()) {
                input_skip = model_json["in_skip"].get<int>();
                if (input_skip > 1)
                    throw std::invalid_argument("Values for in_skip > 1 are not supported");
            }
            else {
                input_skip = 0;
            }

            if (model_json["in_gain"].is_number()) {
                input_gain = DB_CO(model_json["in_gain"].get<float>());
            }
            else {
                input_gain = 1.0f;
            }

            if (model_json["out_gain"].is_number()) {
                output_gain = DB_CO(model_json["out_gain"].get<float>());
            }
            else {
                output_gain = 1.0f;
            }
        }
        catch (const std::exception& e) {
            d_stderr2("Unable to load json, error: %s", e.what());
            return;
        }

        std::unique_ptr<DynamicModel> newmodel = std::make_unique<DynamicModel>();

        try {
            if (! custom_model_creator (model_json, newmodel->variant))
                throw std::runtime_error ("Unable to identify a known model architecture!");

            std::visit (
                [&model_json] (auto&& custom_model)
                {
                    using ModelType = std::decay_t<decltype (custom_model)>;
                    if constexpr (! std::is_same_v<ModelType, NullModel>)
                    {
                        custom_model.parseJson (model_json, true);
                        custom_model.reset();
                    }
                },
                newmodel->variant);
        }
        catch (const std::exception& e) {
            d_stderr2("Error loading model: %s", e.what());
            return;
        }

        // save extra info
        newmodel->input_skip = input_skip != 0;
        newmodel->input_gain = input_gain;
        newmodel->output_gain = output_gain;

        // Pre-buffer to avoid "clicks" during initialization
        float out[2048] = {};
        applyModel(newmodel.get(), out, 2048);

        // swap active model
        DynamicModel* const oldmodel = model;
        model = newmodel.release();

        // if processing, wait for process cycle to complete
        while (oldmodel != nullptr && activeModel.load())
            d_msleep(1);

        delete oldmodel;
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Cabinet loader */

    void loadDefaultCabinet()
    {
        using namespace Files;

        uint channels;
        uint sampleRate;
        drwav_uint64 numFrames;
        float* const ir = drwav_open_memory_and_read_pcm_frames_f32(US_Double_Nrm_CabData,
                                                                    US_Double_Nrm_CabDataSize,
                                                                    &channels,
                                                                    &sampleRate,
                                                                    &numFrames,
                                                                    nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(ir != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(channels == 1,);

        loadCabinet(channels, sampleRate, numFrames, ir);
    }

    void loadCabinetFromFile(const char* const filename)
    {
        uint channels;
        uint sampleRate;
        drwav_uint64 numFrames;
        const size_t valuelen = std::strlen(filename);

        float* ir;
        if (::strncasecmp(filename + std::max(0, static_cast<int>(valuelen) - 5), ".flac", 5) == 0)
            ir = drflac_open_file_and_read_pcm_frames_f32(filename, &channels, &sampleRate, &numFrames, nullptr);
        else
            ir = drwav_open_file_and_read_pcm_frames_f32(filename, &channels, &sampleRate, &numFrames, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(ir != nullptr,);

        loadCabinet(channels, sampleRate, numFrames, ir);

        cabsimFilename = filename;
    }

    void loadCabinet(const uint channels, const uint sampleRate, drwav_uint64 numFrames, float* const ir)
    {
        if (channels != 1)
        {
            for (drwav_uint64 i = 0, j = 0; i < numFrames; ++i, j += channels)
                ir[j] = ir[i];

            numFrames /= channels;
        }

        d_stdout("Loading cabinet with %u channels, %u Hz sample rate and %lu frames",
                 channels, sampleRate, (ulong)numFrames);

        const double hostSampleRate = getSampleRate();
        float* irBuf = ir;

        if (sampleRate != hostSampleRate)
        {
            r8b::CDSPResampler16IR resampler(sampleRate, hostSampleRate, numFrames);
            const int numResampledFrames = resampler.getMaxOutLen(0);
            DISTRHO_SAFE_ASSERT_RETURN(numResampledFrames > 0,);

            float* const irBufResampled = new float[numResampledFrames];
            resampler.oneshot(ir, numFrames, irBufResampled, numResampledFrames);
            irBuf = irBufResampled;

            numFrames = numResampledFrames;
        }

        TwoStageThreadedConvolver* const newConvolver = new TwoStageThreadedConvolver();
        newConvolver->init(irBuf, numFrames);

        if (irBuf != ir)
            delete[] irBuf;

        drwav_free(ir, nullptr);

        // swap active cabsim
        TwoStageThreadedConvolver* const oldcabsim = cabsim;
        cabsim = newConvolver;

        // if processing, wait for process cycle to complete
        while (oldcabsim != nullptr && activeConvolver.load())
            d_msleep(1);

        delete oldcabsim;
    }

   #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
   /* -----------------------------------------------------------------------------------------------------------------
    * Audio file loader */

    void loadAudioFile(const char* const filename)
    {
        uint channels;
        uint sampleRate;
        drwav_uint64 numFrames;
        float* data;
        if (::strncasecmp(filename + std::max(0, static_cast<int>(std::strlen(filename)) - 5), ".flac", 5) == 0)
            data = drflac_open_file_and_read_pcm_frames_f32(filename, &channels, &sampleRate, &numFrames, nullptr);
        else
            data = drwav_open_file_and_read_pcm_frames_f32(filename, &channels, &sampleRate, &numFrames, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(data != nullptr,);

        // use left channel if not mono
        if (channels != 1)
        {
            for (drwav_uint64 i=0, j=0; i<numFrames; ++i, j+=channels)
                data[i] = data[j];
            numFrames /= channels;
        }

        const double hostSampleRate = getSampleRate();

        float* dataResampled;
        if (sampleRate != hostSampleRate)
        {
            r8b::CDSPResampler24 resampler(sampleRate, hostSampleRate, numFrames);
            const int numResampledFrames = resampler.getMaxOutLen(0);
            DISTRHO_SAFE_ASSERT_RETURN(numResampledFrames > 0,);

            dataResampled = new float[numResampledFrames];
            resampler.oneshot(data, numFrames, dataResampled, numResampledFrames);

            numFrames = numResampledFrames;
        }
        else
        {
            dataResampled = data;
        }

        if (dataResampled != data)
            drwav_free(data, nullptr);

        // swap active cabsim
        AudioFile* const newaudiofile = new AudioFile(dataResampled, numFrames, dataResampled != data);
        AudioFile* const oldaudiofile = audiofile;
        audiofile = newaudiofile;

        // if processing, wait for process cycle to complete
        while (oldaudiofile != nullptr && activeAudiofile.load())
            d_msleep(1);

        delete oldaudiofile;
    }
   #endif

   /* -----------------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Activate this plugin.
    */
    void activate() override
    {
        aida.pregain.clearToTarget();
        aida.mastergain.clearToTarget();
        bypassGain.clearToTarget();
        cabsimGain.clearToTarget();
        resetMeterIn.store(true);
        resetMeterOut.store(true);

        if (model != nullptr)
        {
            activeModel.store(true);

            std::visit (
                [] (auto&& custom_model)
                {
                    using ModelType = std::decay_t<decltype (custom_model)>;
                    if constexpr (! std::is_same_v<ModelType, NullModel>)
                    {
                        custom_model.reset();
                    }
                },
                model->variant);

            activeModel.store(false);
        }
    }

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** inputs, float** outputs, uint32_t numSamples) override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        const float* const in = inputs[0];
       #endif
        /* */ float* const out = outputs[0];

        // optimize for non-denormal usage
        const ScopedDenormalDisable sdd;
        for (uint32_t i = 0; i < numSamples; ++i)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            if (!std::isfinite(in[i]))
                __builtin_unreachable();
           #endif
            if (!std::isfinite(out[i]))
                __builtin_unreachable();
        }

       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
        if (audiofile != nullptr)
        {
            activeAudiofile.store(true);
            const uint32_t numPartialSamples = std::min((uint32_t)(audiofile->numFrames - audiofile->currentFrame), numSamples);
            std::memcpy(bypassInplaceBuffer, audiofile->buffer + audiofile->currentFrame, sizeof(float) * numPartialSamples);

            if (numSamples != numPartialSamples)
            {
                const uint32_t extraSamples = numSamples - numPartialSamples;
                std::memcpy(bypassInplaceBuffer, audiofile->buffer, sizeof(float) * extraSamples);
                audiofile->currentFrame = extraSamples;
            }
            else
            {
                audiofile->currentFrame += numSamples;
            }
            activeAudiofile.store(false);
        }
        else
       #endif
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            // Copy input for bypass buffer
            std::memcpy(bypassInplaceBuffer, in, sizeof(float)*numSamples);
           #else
            std::memset(bypassInplaceBuffer, 0, sizeof(float)*numSamples);
           #endif
        }

        // peak meters
        float meterIn, meterOut;

        if (resetMeterIn.exchange(false))
            meterIn = DB_CO(kMinimumMeterDb);
        else
            meterIn = parameters[kParameterMeterIn];

        if (resetMeterOut.exchange(false))
            meterOut = DB_CO(kMinimumMeterDb);
        else
            meterOut = parameters[kParameterMeterOut];

        for (uint32_t i = 0; i < numSamples; ++i)
            meterIn = std::max(meterIn, std::abs(bypassInplaceBuffer[i]));

        // High frequencies roll-off (lowpass)
        applyBiquadFilter(aida.in_lpf, out, bypassInplaceBuffer, numSamples);

        // Pre-gain
        applyGainRamp(aida.pregain, out, numSamples);

        // Equalizer section
        if (!aida.eq_bypass && aida.eq_pos == kEqPre)
            applyToneControls(aida, out, numSamples);

        if (!aida.net_bypass && model != nullptr)
        {
            activeModel.store(true);
            applyModel(model, out, numSamples);
            activeModel.store(false);
        }

        // DC blocker filter (highpass)
        applyBiquadFilter(aida.dc_blocker, out, numSamples);

        // Cabinet convolution
        if (cabsim != nullptr)
        {
            std::memcpy(cabsimInplaceBuffer, out, sizeof(float)*numSamples);

            activeConvolver.store(true);
            cabsim->process(cabsimInplaceBuffer, out, numSamples);
            activeConvolver.store(false);

            // cabsim smooth bypass and -12dB compensation
            for (uint32_t i = 0; i < numSamples; ++i)
            {
                const float b = cabsimGain.next();
                out[i] = out[i] * b + cabsimInplaceBuffer[i] * (kCabinetMaxGain - b);
            }
        }

        // Equalizer section
        if (!aida.eq_bypass && aida.eq_pos == kEqPost)
            applyToneControls(aida, out, numSamples);

        // Master volume
        applyGainRamp(aida.mastergain, out, numSamples);

        // Bypass and output meter
        for (uint32_t i = 0; i < numSamples; ++i)
        {
            const float b = bypassGain.next();
            out[i] = out[i] * b + bypassInplaceBuffer[i] * (1.f - b);
            meterOut = std::max(meterOut, std::abs(out[i]));
        }

        parameters[kParameterMeterIn] = meterIn;
        parameters[kParameterMeterOut] = meterOut;

       #if DISTRHO_PLUGIN_VARIANT_STANDALONE
        std::memcpy(outputs[1], out, sizeof(float)*numSamples);
       #endif
    }

    void bufferSizeChanged(const uint newBufferSize) override
    {
        delete[] bypassInplaceBuffer;
        delete[] cabsimInplaceBuffer;
        bypassInplaceBuffer = new float[newBufferSize];
        cabsimInplaceBuffer = new float[newBufferSize];
    }

   /**
      Optional callback to inform the plugin about a sample rate change.@n
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(const double newSampleRate) override
    {
        aida.setSampleRate(parameters, newSampleRate);

        bypassGain.setSampleRate(newSampleRate);
        cabsimGain.setSampleRate(newSampleRate);

        // reload cabsim file
        if (char* const filename = cabsimFilename.getAndReleaseBuffer())
        {
            setState("cabinet", filename);
            std::free(filename);
        }
        else
        {
            loadDefaultCabinet();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AidaDSPLoaderPlugin)
};

/* --------------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new AidaDSPLoaderPlugin();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
