/*
 * Aida-X DPF plugin
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "DistrhoPluginUtils.hpp"
#include "DistrhoUI.hpp"
#include "DistrhoStandaloneUtils.hpp"

#include "Graphics.hpp"

#include "Layout.hpp"
#include "Widgets.hpp"

#if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS != 0
# include "Blendish.hpp"
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

enum ButtonIds {
    kButtonLoadModel = 1001,
    kButtonLoadCabinet,
  #if DISTRHO_PLUGIN_VARIANT_STANDALONE
   #if DISTRHO_PLUGIN_NUM_INPUTS != 0
    kButtonEnableMicInput,
   #else
    kButtonAudioFileStartId = 2001,
   #endif
  #endif
};

enum MicInputState {
    kMicInputUnsupported,
    kMicInputSupported,
    kMicInputEnabled,
    kMicInputJACK
};

class AidaDSPLoaderUI : public UI,
                        public ButtonEventHandler::Callback,
                        public KnobEventHandler::Callback
                     #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS != 0
                      , public BlendishComboBox::Callback
                      , public BlendishToolButton::Callback
                     #endif
{
    float parameters[kNumParameters];

    struct {
        NanoImage aida;
        NanoImage ax;
        NanoImage background;
        NanoImage knob;
        NanoImage scale;
    } images;

    struct {
        NanoImage amp;
        NanoImage ampOn;
        NanoImage cab;
        NanoImage cabOn;
    } icons;

    struct {
        ScopedPointer<AidaKnob> pregain;
        ScopedPointer<AidaKnob> bass;
        ScopedPointer<AidaKnob> middle;
        ScopedPointer<AidaKnob> treble;
        ScopedPointer<AidaKnob> depth;
        ScopedPointer<AidaKnob> presence;
        ScopedPointer<AidaKnob> master;
    } knobs;

    struct {
        ScopedPointer<AidaPluginSwitch> bypass;
        ScopedPointer<AidaPluginSwitch> eqpos;
        ScopedPointer<AidaPluginSwitch> midtype;
    } switches;

    struct {
        ScopedPointer<AidaSplitter> s1, s2, s3;
    } splitters;
    
    struct {
        ScopedPointer<AidaFilenameButton> model;
        ScopedPointer<AidaFilenameButton> cabsim;
    } loaders;

  #if DISTRHO_PLUGIN_VARIANT_STANDALONE
   #if DISTRHO_PLUGIN_NUM_INPUTS != 0
    MicInputState micInputState = kMicInputUnsupported;
    ScopedPointer<BlendishSubWidgetSharedContext> blendishParent;
    ScopedPointer<BlendishToolButton> micButton;
    ScopedPointer<BlendishComboBox> bufferSizeComboBox;
   #else
    ScopedPointer<AidaFileList> audioFileList;
   #endif
  #endif

    HorizontalLayout subwidgetsLayout;

    enum {
        kFileLoaderNull,
        kFileLoaderModel,
        kFileLoaderImpulse,
    } fileLoaderMode = kFileLoaderNull;

    String aboutLabel;
    String lastDirModel;
    String lastDirCabinet;

public:
    /* constructor */
    AidaDSPLoaderUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        // Initialize parameters to their defaults
        for (uint i=0; i<kNumParameters; ++i)
            parameters[i] = kParameters[i].ranges.def;

        // Load resources
        using namespace Graphics;
        images.aida = createImageFromMemory(aidaData, aidaDataSize, IMAGE_GENERATE_MIPMAPS);
        images.ax = createImageFromMemory(axData, axDataSize, IMAGE_GENERATE_MIPMAPS);
#ifdef DGL_USE_GLES
        images.background = createImageFromMemory(background_p2Data, background_p2DataSize, IMAGE_REPEAT_X|IMAGE_REPEAT_Y);
#else
        images.background = createImageFromMemory(backgroundData, backgroundDataSize, IMAGE_REPEAT_X|IMAGE_REPEAT_Y);
#endif
        images.knob = createImageFromMemory(knobData, knobDataSize, IMAGE_GENERATE_MIPMAPS);
        images.scale = createImageFromMemory(scaleData, scaleDataSize, IMAGE_GENERATE_MIPMAPS);

        icons.amp = createImageFromMemory(ax_ampData, ax_ampDataSize, IMAGE_GENERATE_MIPMAPS);
        icons.ampOn = createImageFromMemory(ax_amp_onData, ax_amp_onDataSize, IMAGE_GENERATE_MIPMAPS);
        icons.cab = createImageFromMemory(ax_cabData, ax_cabDataSize, IMAGE_GENERATE_MIPMAPS);
        icons.cabOn = createImageFromMemory(ax_cab_onData, ax_cab_onDataSize, IMAGE_GENERATE_MIPMAPS);

        // fontFaceId(createFontFromMemory("conthrax", conthrax_sbData, conthrax_sbDataSize, false));
        loadSharedResources();

        // Create subwidgets
        knobs.pregain = new AidaKnob(this, this, images.knob, images.scale, kParameterPREGAIN);
        knobs.bass = new AidaKnob(this, this, images.knob, images.scale, kParameterBASSGAIN);
        knobs.middle = new AidaKnob(this, this, images.knob, images.scale, kParameterMIDGAIN);
        knobs.treble = new AidaKnob(this, this, images.knob, images.scale, kParameterTREBLEGAIN);
        knobs.depth = new AidaKnob(this, this, images.knob, images.scale, kParameterDEPTH);
        knobs.presence = new AidaKnob(this, this, images.knob, images.scale, kParameterPRESENCE);
        knobs.master = new AidaKnob(this, this, images.knob, images.scale, kParameterMASTER);

        switches.bypass = new AidaPluginSwitch(this, this, kParameterGLOBALBYPASS);
        switches.eqpos = new AidaPluginSwitch(this, this, kParameterEQPOS);
        switches.midtype = new AidaPluginSwitch(this, this, kParameterMTYPE);

        splitters.s1 = new AidaSplitter(this);
        splitters.s2 = new AidaSplitter(this);
        splitters.s3 = new AidaSplitter(this);

        loaders.model = new AidaFilenameButton(this, this, kParameterNETBYPASS, kButtonLoadModel, "model", icons.amp, icons.ampOn);
        loaders.model->setFilename(kDefaultModelName);

        loaders.cabsim = new AidaFilenameButton(this, this, kParameterCABSIMBYPASS, kButtonLoadCabinet, "cabinet IR", icons.cab, icons.cabOn);
        loaders.cabsim->setFilename(kDefaultCabinetName);

      #if DISTRHO_PLUGIN_VARIANT_STANDALONE
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        if (isUsingNativeAudio())
        {
            if (supportsAudioInput())
            {
                blendishParent = new BlendishSubWidgetSharedContext(this);

                micButton = new BlendishToolButton(blendishParent);
                micButton->setCallback(this);
                micButton->setId(kButtonEnableMicInput);
                micButton->setLabel("Enable Input");
                micInputState = kMicInputSupported;

                bufferSizeComboBox = new BlendishComboBox(blendishParent);
                bufferSizeComboBox->setCallback(this);
                bufferSizeComboBox->setDefaultLabel("Buffer Size: " + String(getBufferSize()));
                bufferSizeComboBox->addMenuItem("128");
                bufferSizeComboBox->addMenuItem("256");
                bufferSizeComboBox->addMenuItem("512");
                bufferSizeComboBox->addMenuItem("1024");
                bufferSizeComboBox->addMenuItem("2048");
                bufferSizeComboBox->addMenuItem("4096");
                bufferSizeComboBox->addMenuItem("8192");
            }
        }
        else
        {
            micInputState = kMicInputJACK;
        }
       #else
        audioFileList = new AidaFileList(this, this, kButtonAudioFileStartId);
        audioFileList->setFilename(0, kAudioLoopFilenames[0], true);
        audioFileList->setFilename(1, kAudioLoopFilenames[1]);
        audioFileList->setFilename(2, kAudioLoopFilenames[2]);
       #endif
      #endif

        // Setup subwidgets layout
        subwidgetsLayout.widgets.push_back({ switches.bypass, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.pregain, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s1, Fixed });
        subwidgetsLayout.widgets.push_back({ switches.eqpos, Fixed });
        subwidgetsLayout.widgets.push_back({ switches.midtype, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.bass, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.middle, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.treble, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s2, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.depth, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.presence, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s3, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.master, Fixed });
        repositionWidgets();

        // adjust size
        const double scaleFactor = getScaleFactor();
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                               DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);

        if (scaleFactor != 1.0)
            setSize(DISTRHO_UI_DEFAULT_WIDTH*scaleFactor, DISTRHO_UI_DEFAULT_HEIGHT*scaleFactor);

        aboutLabel = "AIDA-X ";
        aboutLabel += getPluginFormatName();
        aboutLabel += " ";
        aboutLabel += kVersionString;
    }

protected:
   /* -----------------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(const uint32_t index, const float value) override
    {
        parameters[index] = value;

        switch (static_cast<Parameters>(index))
        {
        case kParameterINLPF:
            // TODO
            break;
        case kParameterPREGAIN:
            knobs.pregain->setValue(value, false);
            break;
        case kParameterNETBYPASS:
            loaders.model->setChecked(value < 0.5f);
            break;
        case kParameterEQBYPASS:
            // TODO
            break;
        case kParameterEQPOS:
            switches.eqpos->setChecked(value > 0.5f, false);
            break;
        case kParameterBASSGAIN:
            knobs.bass->setValue(value, false);
            break;
        case kParameterMIDGAIN:
            knobs.middle->setValue(value, false);
            break;
        case kParameterMTYPE:
            switches.midtype->setChecked(value > 0.5f, false);
            break;
        case kParameterTREBLEGAIN:
            knobs.treble->setValue(value, false);
            break;
        case kParameterDEPTH:
            knobs.depth->setValue(value, false);
            break;
        case kParameterPRESENCE:
            knobs.presence->setValue(value, false);
            break;
        case kParameterMASTER:
            knobs.master->setValue(value, false);
            break;
        case kParameterCABSIMBYPASS:
            loaders.cabsim->setChecked(value < 0.5f);
            break;
        case kParameterGLOBALBYPASS:
            switches.bypass->setChecked(value < 0.5f, false);
            break;
        case kParameterBASSFREQ:
        case kParameterMIDFREQ:
        case kParameterMIDQ:
        case kParameterTREBLEFREQ:
        case kParameterCount:
            break;
        }
    }

    void stateChanged(const char* const key, const char* const value) override
    {
        const bool isDefault = value == nullptr || value[0] == '\0' || std::strcmp(value, "default") == 0;

        if (std::strcmp(key, "json") == 0)
            return loaders.model->setFilename(isDefault ? kDefaultModelName : value);
        if (std::strcmp(key, "cabinet") == 0)
            return loaders.cabsim->setFilename(isDefault ? kDefaultCabinetName : value);
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();
        const double scaleFactor = getScaleFactor();

        const double cornerRadius = 12 * scaleFactor;
        const double marginHead = 12 * scaleFactor;
        const double widthPedal = kPedalWidth * scaleFactor;
        const double heightPedal = kPedalHeight * scaleFactor;
        const double widthHead = widthPedal - marginHead * 2;
        const double heightHead = 177 * scaleFactor;
        const double marginHorizontal = kPedalMargin * scaleFactor + (getWidth() - DISTRHO_UI_DEFAULT_WIDTH * scaleFactor) / 2;
        const double marginVertical = kPedalMarginTop * scaleFactor;

        const Size<uint> headBgSize(images.background.getSize() / 2 * scaleFactor);

        // outer bounds gradient
        beginPath();
        rect(0, 0, width, height);
        fillPaint(linearGradient(0, 0, 0, height,
                                 Color(0xcd, 0xff, 0x05).minus(50).invert(),
                                 Color(0x8b, 0xf7, 0x00).minus(50).invert()));
        fill();

        // outer bounds inner shadow matching host color, if provided
        Color bgColor;
        if (const uint hostBgColor = getBackgroundColor())
        {
            const int red   = (hostBgColor >> 24) & 0xff;
            const int green = (hostBgColor >> 16) & 0xff;
            const int blue  = (hostBgColor >>  8) & 0xff;
            bgColor = Color(red, green, blue);
        }
        else
        {
            bgColor = Color(0,0,0);
        }

        fillPaint(boxGradient(scaleFactor, scaleFactor, width-scaleFactor*2, height-scaleFactor*2,
                              cornerRadius/2, cornerRadius/4, bgColor.withAlpha(0.f), bgColor));
        fill();

        // outer bounds pattern
        fillPaint(imagePattern(0,
                               0,
                               headBgSize.getWidth(),
                               headBgSize.getHeight(),
                               0.f, images.background, 1.f));
        fill();

        // box shadow
        beginPath();
        rect(marginHorizontal/2, marginVertical/2, marginHorizontal+widthPedal, marginVertical+heightPedal);
        fillPaint(boxGradient(marginHorizontal, marginVertical, widthPedal, heightPedal, cornerRadius, cornerRadius, Color(0,0,0,1.f), Color(0,0,0,0.f)));
        fill();

        // .rt-neural .grid
        beginPath();
        roundedRect(marginHorizontal, marginVertical, widthPedal, heightPedal, cornerRadius);
        fillPaint(linearGradient(marginHorizontal, 0,
                                 marginHorizontal + widthPedal * 0.52f, 0,
                                 Color(28, 23, 12),
                                 Color(42, 34, 15)));
        fill();

        beginPath();
        roundedRect(marginHorizontal, marginVertical, widthPedal, heightPedal, cornerRadius);
        fillPaint(linearGradient(marginHorizontal + widthPedal * 0.50f, 0,
                                 marginHorizontal + widthPedal, 0,
                                 Color(42, 34, 15),
                                 Color(19, 19, 19)));
        fill();

        // extra
        strokeColor(Color(150, 150, 150, 0.25f));
        stroke();

        // .rt-neural .background_head
        beginPath();
        roundedRect(marginHorizontal + marginHead,
                    marginVertical + marginHead,
                    widthHead,
                    heightHead,
                    cornerRadius);
        fillPaint(linearGradient(marginHorizontal + marginHead,
                                 marginVertical + marginHead,
                                 marginHorizontal + marginHead,
                                 marginVertical + heightHead,
                                 Color(0x8b, 0xf7, 0x00),
                                 Color(0xcd, 0xff, 0x05)));
        fill();

        fillPaint(imagePattern(marginHorizontal + marginHead,
                               marginVertical + marginHead,
                               headBgSize.getWidth(),
                               headBgSize.getHeight(),
                               0.f, images.background, 1.f));
        fill();

        fillPaint(boxGradient(marginHorizontal + marginHead,
                              marginVertical + marginHead,
                              widthHead,
                              heightHead,
                              cornerRadius,
                              12 * scaleFactor,
                              Color(0, 0, 0, 0.f), Color(0, 0, 0)));
        fill();

        // .rt-neural .brand
        const Size<uint> aidaLogoSize(111 * scaleFactor, 25 * scaleFactor);

        save();
        translate(marginHorizontal + marginHead * 2, marginVertical + headBgSize.getHeight());
        beginPath();
        rect(0, 0, aidaLogoSize.getWidth(), aidaLogoSize.getHeight());
        fillPaint(imagePattern(0, 0, aidaLogoSize.getWidth(), aidaLogoSize.getHeight(), 0.f, images.aida, 1.f));
        fill();
        restore();

        // .rt-neural .plate
        const Size<uint> axLogoSize(100 * scaleFactor * 1548 / 727,
                                    100 * scaleFactor);

        save();
        translate(marginHorizontal + widthPedal/2 - axLogoSize.getWidth()/2, marginVertical + marginHead + headBgSize.getHeight() / 6);
        beginPath();
        rect(0, 0, axLogoSize.getWidth(), axLogoSize.getHeight());
        fillPaint(imagePattern(0, 0, axLogoSize.getWidth(), axLogoSize.getHeight(), 0.f, images.ax, 1.f));
        fill();
        restore();

        fillColor(Color(0x0c, 0x2f, 0x03, 0.686f));
        fontSize(24 * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_BASELINE);
        text(marginHorizontal + widthPedal/2, marginVertical + heightHead - marginHead, "AI CRAFTED TONE", nullptr);

        fillColor(Color(1.f,1.f,1.f));
        fontSize((kSubWidgetsFontSize + 2) * scaleFactor);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        text(marginHorizontal + widthPedal - 10 * scaleFactor, marginVertical/2, aboutLabel, nullptr);

       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS != 0
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

        const double micx = marginHorizontal + (micButton != nullptr ? micButton->getWidth() : 0) + 10 * scaleFactor;
        switch (micInputState)
        {
        case kMicInputUnsupported:
            text(micx, marginVertical/2, "Input Unsupported", nullptr);
            break;
        case kMicInputSupported:
            text(micx, marginVertical/2, "Please enable Input...", nullptr);
            break;
        case kMicInputEnabled:
            text(micx, marginVertical/2, "Input enabled", nullptr);
            break;
        case kMicInputJACK:
            text(micx, marginVertical/2, "Input always enabled (using JACK)", nullptr);
            break;
        }
       #endif
    }

    void onResize(const ResizeEvent& event) override
    {
        UI::onResize(event);
        repositionWidgets();
       #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS != 0
        if (blendishParent != nullptr)
            blendishParent->setSize(event.size);
       #endif
    }

    void repositionWidgets()
    {
        const double scaleFactor = getScaleFactor();
        const double widthPedal = kPedalWidth * scaleFactor;
        const double heightPedal = kPedalHeight * scaleFactor;
        const double marginHorizontal = kPedalMargin * scaleFactor + (getWidth() - DISTRHO_UI_DEFAULT_WIDTH * scaleFactor) / 2;
        const double marginTop = kPedalMarginTop * scaleFactor;
        const double heightHead = 177 * scaleFactor;
        const double margin = 15 * scaleFactor;

        const uint unusedSpace = widthPedal
                               - (AidaKnob::kScaleSize * scaleFactor * 7)
                               - (AidaPluginSwitch::kFullWidth  * scaleFactor * 3)
                               - (AidaSplitter::kLineWidth * scaleFactor * 3);
        const uint padding = unusedSpace / 14;
        const uint maxHeight = subwidgetsLayout.setAbsolutePos(marginHorizontal + margin,
                                                               marginTop + heightPedal - margin - kSubWidgetsFullHeight * scaleFactor,
                                                               padding);
        subwidgetsLayout.setSize(maxHeight, 0);

        const double loadersX = marginHorizontal + widthPedal * 2 / 3;
        const double loadersY = marginTop + heightPedal - heightHead;

        loaders.model->setAbsolutePos(loadersX, loadersY - margin / 2 - loaders.cabsim->getHeight());
        loaders.model->setWidth(widthPedal / 3 - margin * 2);

        loaders.cabsim->setAbsolutePos(loadersX, loadersY);
        loaders.cabsim->setWidth(widthPedal / 3 - margin * 2);

      #if DISTRHO_PLUGIN_VARIANT_STANDALONE
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        if (blendishParent != nullptr)
        {
            micButton->setAbsolutePos(marginHorizontal,
                                      marginTop/2 - micButton->getHeight()/2);
            bufferSizeComboBox->setAbsolutePos(getWidth() / 2 - bufferSizeComboBox->getWidth()/2,
                                               marginTop/2 - bufferSizeComboBox->getHeight()/2);
        }
       #else
        audioFileList->setAbsolutePos(loadersX, marginTop + margin * 3 / 2);
        audioFileList->setWidth(widthPedal / 3 - margin * 2);
       #endif
      #endif
    }

    void buttonClicked(SubWidget* const widget, int button) override
    {
        if (button != kMouseButtonLeft)
            return;

        const uint id = widget->getId();

        switch (id)
        {
        case kParameterEQPOS:
        case kParameterMTYPE:
            editParameter(id, true);
            setParameterValue(id, static_cast<AidaPluginSwitch*>(widget)->isChecked() ? 1.f : 0.f);
            editParameter(id, false);
            break;
        case kParameterCABSIMBYPASS:
        case kParameterNETBYPASS:
        case kParameterGLOBALBYPASS:
            editParameter(id, true);
            setParameterValue(id, static_cast<AidaPluginSwitch*>(widget)->isChecked() ? 0.f : 1.f);
            editParameter(id, false);
            break;
        case kButtonLoadModel:
            fileLoaderMode = kFileLoaderModel;
            requestStateFile("json", lastDirModel, "Open AidaDSP model json");
            break;
        case kButtonLoadCabinet:
            fileLoaderMode = kFileLoaderImpulse;
            requestStateFile("cabinet", lastDirCabinet, "Open Cabinet Simulator IR");
            break;
      #if DISTRHO_PLUGIN_VARIANT_STANDALONE
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        case kButtonEnableMicInput:
            if (supportsAudioInput() && !isAudioInputEnabled())
                requestAudioInput();
            break;
       #else
        case kButtonAudioFileStartId:
        case kButtonAudioFileStartId+1:
        case kButtonAudioFileStartId+2:
            setState("audiofile", kAudioLoopFilenames[id - kButtonAudioFileStartId]);
            break;
       #endif
      #endif
        }
    }

    void requestStateFile(const char* const stateKey, const String& lastDir, const char* const title)
    {
       #ifndef DISTRHO_OS_WASM
        if (UI::requestStateFile(stateKey))
            return;
        d_stdout("File through host failed, doing it manually");
       #endif

        DISTRHO_NAMESPACE::FileBrowserOptions opts;
        opts.title = title;

        if (lastDir.isNotEmpty())
            opts.startDir = lastDir;

        if (!openFileBrowser(opts))
        {
            d_stdout("Failed to open a file dialog!");
        }
    }

    void knobDragStarted(SubWidget* const widget) override
    {
        editParameter(widget->getId(), true);
    }

    void knobDragFinished(SubWidget* const widget) override
    {
        editParameter(widget->getId(), false);
    }

    void knobValueChanged(SubWidget* const widget, float value) override
    {
        setParameterValue(widget->getId(), value);
    }

   #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS != 0
    void blendishComboBoxIndexChanged(BlendishComboBox* const comboBox, int) override
    {
        const String label(comboBox->getCurrentLabel());
        comboBox->setCurrentIndex(-1, false);

        if (requestBufferSizeChange(std::atoi(label)))
            bufferSizeComboBox->setDefaultLabel("Buffer Size: " + label);
    }

    void blendishToolButtonClicked(BlendishToolButton* const widget, int button) override
    {
        if (button != kMouseButtonLeft)
            return;

        if (widget->getId() == kButtonEnableMicInput)
            if (supportsAudioInput() && !isAudioInputEnabled())
                requestAudioInput();
    }

    void uiIdle() override
    {
        if (micButton != nullptr)
        {
            const MicInputState newMicInputState = isAudioInputEnabled() ? kMicInputEnabled : kMicInputSupported;

            if (micInputState != newMicInputState)
            {
                micInputState = newMicInputState;
                repaint();
            }
        }
    }
   #endif

   /**
      Window file selected function, called when a path is selected by the user, as triggered by openFileBrowser().
      This function is for plugin UIs to be able to override Window::onFileSelected(const char*).

      This action happens after the user confirms the action, so the file browser dialog will be closed at this point.
      The default implementation does nothing.
    */
    void uiFileBrowserSelected(const char* const filename) override
    {
        if (filename == nullptr)
            return;

        switch (fileLoaderMode)
        {
        case kFileLoaderNull:
            break;
        case kFileLoaderModel:
            // notify DSP
            setState("json", filename);

            // update UI
            loaders.model->setFilename(filename);

            // save dirname for next time
            if (const char* const lastsep = std::strrchr(filename, DISTRHO_OS_SEP))
            {
                lastDirModel = filename;
                lastDirModel.truncate(lastsep - filename);
            }
            break;
        case kFileLoaderImpulse:
            // notify DSP
            setState("cabinet", filename);

            // update UI
            loaders.cabsim->setFilename(filename);

            // save dirname for next time
            if (const char* const lastsep = std::strrchr(filename, DISTRHO_OS_SEP))
            {
                lastDirCabinet = filename;
                lastDirCabinet.truncate(lastsep - filename);
            }
            break;
        }

        fileLoaderMode = kFileLoaderNull;
    }

    // ----------------------------------------------------------------------------------------------------------------

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AidaDSPLoaderUI)
};

/* --------------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new AidaDSPLoaderUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
