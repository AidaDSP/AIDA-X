/*
 * Aida-X DPF plugin
 * Copyright (C) 2022-2023 Massimo Pennazio <maxipenna@libero.it>
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// no audio inputs on web version
#ifdef DISTRHO_OS_WASM
# define DISTRHO_PLUGIN_NUM_INPUTS     0
#else
# define DISTRHO_PLUGIN_NUM_INPUTS     1
#endif

#define DISTRHO_PLUGIN_NUM_OUTPUTS     2
#define DISTRHO_UI_USER_RESIZABLE      0

#define DISTRHO_PLUGIN_VARIANT_PLUGIN     0
#define DISTRHO_PLUGIN_VARIANT_STANDALONE 1

#include "../DistrhoPluginCommon.hpp"

#ifdef DISTRHO_OS_WASM
static constexpr const char* const kAudioLoopFilenames[3] = {
    "/resources/0037.wav",
    "/resources/0045.wav",
    "/resources/0050.wav",
};
#endif
