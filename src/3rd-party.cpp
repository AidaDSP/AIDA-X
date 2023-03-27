/*
 * Aida-X DPF plugin
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "r8brain/pffft.cpp"
#include "r8brain/r8bbase.cpp"
