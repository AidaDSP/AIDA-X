/*
 * AIDA-X DPF plugin
 * Copyright (C) 2022-2023 Massimo Pennazio <maxipenna@libero.it>
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "DistrhoDetails.hpp"

static constexpr const char* const kVersionString = "v0.1.0";
static constexpr const uint32_t kVersionNumber = d_version(0, 1, 0);

#define DISTRHO_PLUGIN_BRAND   "AIDA DSP"
#define DISTRHO_PLUGIN_NAME    "AIDA-X"
#define DISTRHO_PLUGIN_URI     "http://aidadsp.cc/plugins/aidadsp-bundle/rt-neural-loader"
#define DISTRHO_PLUGIN_CLAP_ID "cc.aidadsp.rt-neural-loader"

#define DISTRHO_PLUGIN_HAS_UI          1
#define DISTRHO_PLUGIN_IS_RT_SAFE      1
#define DISTRHO_PLUGIN_WANT_PROGRAMS   0
#define DISTRHO_PLUGIN_WANT_STATE      1
#define DISTRHO_UI_FILE_BROWSER        1
#define DISTRHO_UI_USE_NANOVG          1

#define DISTRHO_PLUGIN_CLAP_FEATURES   "audio-effect", "multi-effects", "mono"
#define DISTRHO_PLUGIN_LV2_CATEGORY    "lv2:SimulatorPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Dynamics|Mono"

// known and defined in advance
static constexpr const uint kPedalWidth = 900;
static constexpr const uint kPedalHeight = 318;
#ifndef MOD_BUILD
static constexpr const uint kPedalMargin = 20;
static constexpr const uint kPedalMarginTop = 40;
#else
static constexpr const uint kPedalMargin = 0;
static constexpr const uint kPedalMarginTop = 0;
#endif

#define DISTRHO_UI_DEFAULT_WIDTH  (kPedalWidth + kPedalMargin * 2)
#define DISTRHO_UI_DEFAULT_HEIGHT (kPedalHeight + kPedalMargin + kPedalMarginTop)

static constexpr const char* const kDefaultModelName = "deerinkstudios-tw40-ac-clean.json";
static constexpr const char* const kDefaultCabinetName = "Twin Rev Nrm N6 Cab IR.wav";

static constexpr const float kMinimumMeterDb = -60.f;

enum Parameters {
    kParameterINLPF,
    kParameterPREGAIN,
    kParameterNETBYPASS,
    kParameterEQBYPASS,
    kParameterEQPOS,
    kParameterBASSGAIN,
    kParameterBASSFREQ,
    kParameterMIDGAIN,
    kParameterMIDFREQ,
    kParameterMIDQ,
    kParameterMTYPE,
    kParameterTREBLEGAIN,
    kParameterTREBLEFREQ,
    kParameterDEPTH,
    kParameterPRESENCE,
    kParameterMASTER,
    kParameterCABSIMBYPASS,
    kParameterGLOBALBYPASS,
    kParameterPARAM1,
    kParameterPARAM2,
    kParameterModelInputSize,
    kParameterMeterIn,
    kParameterMeterOut,
    kParameterCount
};

enum States {
    kStateModelFile,
    kStateImpulseFile,
   #if DISTRHO_PLUGIN_VARIANT_STANDALONE && DISTRHO_PLUGIN_NUM_INPUTS == 0
    kStateAudioFile,
   #endif
    kStateCount
};

enum EqPos {
    kEqPost,
    kEqPre
};

enum MidEqType {
    kMidEqPeak,
    kMidEqBandpass
};

static ParameterEnumerationValue kEQPOS[2] = {
    { kEqPost, "POST" },
    { kEqPre, "PRE" }
};

static ParameterEnumerationValue kMTYPE[2] = {
    { kMidEqPeak, "PEAK" },
    { kMidEqBandpass, "BANDPASS" }
};

static ParameterEnumerationValue kBYPASS[2] = {
    { 0.f, "ON" },
    { 1.f, "OFF" }
};

static ParameterEnumerationValue kModelInSize[4] = {
    { 0.f, "ERROR" },
    { 1.f, "SNAPSHOT" },
    { 2.f, "WITH 1 PARAM" },
    { 3.f, "WITH 2 PARAMS" }
};

static const Parameter kParameters[] = {
    { kParameterIsAutomatable, "ANTIALIASING", "ANTIALIASING", "%", 66.216f, 0.f, 100.f, },
    { kParameterIsAutomatable, "PREGAIN", "PREGAIN", "dB", 0.f, -12.f, 3.f, },
    { kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger, "NETBYPASS", "NETBYPASS", "", 0.f, 0.f, 1.f, },
    { kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger, "EQBYPASS", "EQBYPASS", "", 0.f, 0.f, 1.f, },
    { kParameterIsAutomatable|kParameterIsInteger, "EQPOS", "EQPOS", "", 0.f, 0.f, 1.f, ARRAY_SIZE(kEQPOS), kEQPOS },
    { kParameterIsAutomatable, "BASS", "BASS", "dB", 0.f, -8.f, 8.f, },
    { kParameterIsAutomatable, "BFREQ", "BFREQ", "Hz", 305.f, 75.f, 600.f, },
    { kParameterIsAutomatable, "MID", "MID", "dB", 0.f, -8.f, 8.f, },
    { kParameterIsAutomatable, "MFREQ", "MFREQ", "Hz", 750.f, 150.f, 5000.f, },
    { kParameterIsAutomatable, "MIDQ", "MIDQ", "", 0.707f, 0.2f, 5.f, },
    { kParameterIsAutomatable|kParameterIsInteger, "MTYPE", "MTYPE", "", 0.f, 0.f, 1.f, ARRAY_SIZE(kMTYPE), kMTYPE },
    { kParameterIsAutomatable, "TREBLE", "TREBLE", "dB", 0.f, -8.f, 8.f, },
    { kParameterIsAutomatable, "TFREQ", "TFREQ", "Hz", 2000.f, 1000.f, 4000.f, },
    { kParameterIsAutomatable, "DEPTH", "DEPTH", "dB", 0.f, -8.f, 8.f, },
    { kParameterIsAutomatable, "PRESENCE", "PRESENCE", "dB", 0.f, -8.f, 8.f, },
    { kParameterIsAutomatable, "MASTER", "MASTER", "dB", 0.f, -15.f, 15.f, },
    { kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger, "CABSIMBYPASS", "CABSIMBYPASS", "", 0.f, 0.f, 1.f, },
    { kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger, "Bypass", "dpf_bypass", "", 0.f, 0.f, 1.f, ARRAY_SIZE(kBYPASS), kBYPASS },
    { kParameterIsAutomatable, "PARAM1", "PARAM1", "", 0.f, 0.f, 1.f, },
    { kParameterIsAutomatable, "PARAM2", "PARAM2", "", 0.f, 0.f, 1.f, },
    { kParameterIsOutput, "Model Input Size", "ModelInSize", "", 0.f, 0.f, 3.f, ARRAY_SIZE(kModelInSize), kModelInSize },
    { kParameterIsOutput, "Meter In", "MeterIn", "dB", 0.f, 0.f, 2.f, },
    { kParameterIsOutput, "Meter Out", "MeterOut", "dB", 0.f, 0.f, 2.f, },
};

static constexpr const uint kNumParameters = ARRAY_SIZE(kParameters);

static_assert(kNumParameters == kParameterCount, "Matched num params");
