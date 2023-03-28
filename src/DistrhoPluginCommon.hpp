/*
 * Aida-X DPF plugin
 * Copyright (C) 2022-2023 Massimo Pennazio <maxipenna@libero.it>
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "DistrhoDetails.hpp"

#define DISTRHO_PLUGIN_BRAND   "AIDA-X"
#define DISTRHO_PLUGIN_NAME    "RT Neural Loader"
#define DISTRHO_PLUGIN_URI     "http://aidadsp.cc/plugins/aidadsp-bundle/rt-neural-loader"
#define DISTRHO_PLUGIN_CLAP_ID "cc.aidadsp.rt-neural-loader"

#define DISTRHO_PLUGIN_HAS_UI          1
#define DISTRHO_PLUGIN_IS_RT_SAFE      1
#define DISTRHO_PLUGIN_WANT_PROGRAMS   0
#define DISTRHO_PLUGIN_WANT_STATE      1
#define DISTRHO_UI_FILE_BROWSER        1
#define DISTRHO_UI_USE_NANOVG          1
#define DISTRHO_UI_USER_RESIZABLE      1

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:SimulatorPlugin"

// known and defined in advance
static const constexpr uint kPedalWidth = 900;
static const constexpr uint kPedalHeight = 328;
static const constexpr uint kPedalMargin = 40;

#define DISTRHO_UI_DEFAULT_WIDTH  (kPedalWidth + kPedalMargin * 2)
#define DISTRHO_UI_DEFAULT_HEIGHT (kPedalHeight + kPedalMargin * 2)

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
    kParameterCONVOLVERENABLE,
    kParameterGLOBALBYPASS,
    kParameterReportModelType,
    kParameterReportCabinetLength,
    kParameterCount
};

enum States {
    kStateModelFile,
    kStateImpulseFile,
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

enum ReportModelType {
    kReportModelNone,
    kReportModelStandard,
    kReportModelLight,
    kReportModelHeavy
};

static const ParameterEnumerationValue kEQPOS[2] = {
    { kEqPost, "POST" },
    { kEqPre, "PRE" }
};

static const ParameterEnumerationValue kMTYPE[2] = {
    { kMidEqPeak, "PEAK" },
    { kMidEqBandpass, "BANDPASS" }
};

static const ParameterEnumerationValue kBYPASS[2] = {
    { 0.f, "ON" },
    { 1.f, "OFF" }
};

static const Parameter kParameters[] = {
    { kParameterIsAutomatable, "INLPF", "INLPF", "%", 50.f, 25.f, 100.f, },
    { kParameterIsAutomatable, "PREGAIN", "PREGAIN", "dB", -6.f, -12.f, 0.f, },
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
    { kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger, "CONVOLVERENABLE", "CONVOLVERENABLE", "", 0.f, 0.f, 1.f, },
    { kParameterIsAutomatable|kParameterIsBoolean|kParameterIsInteger, "Bypass", "dpf_bypass", "", 0.f, 0.f, 1.f, ARRAY_SIZE(kBYPASS), kBYPASS },
    { kParameterIsOutput, "ReportModelType", "ReportModelType", "", 0.f, 0.f, kReportModelHeavy, },
    { kParameterIsOutput, "ReportCabinetLength", "ReportCabinetLength", "", 0.f, 0.f, 3600.f, },
};

static constexpr const uint kNumParameters = ARRAY_SIZE(kParameters);

static_assert(kNumParameters == kParameterCount, "Matched num params");
