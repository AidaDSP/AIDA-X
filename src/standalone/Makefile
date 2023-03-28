#!/usr/bin/make -f
# Makefile for Aida-X #
# ------------------- #
# Created by falkTX
#

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = Aida-X

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = \
	../3rd-party.cpp \
	../aidadsp-plugin.cpp \
	../Biquad.cpp \
	../../build/Files.cpp \
	../../modules/FFTConvolver/AudioFFT.cpp \
	../../modules/FFTConvolver/FFTConvolver.cpp \
	../../modules/FFTConvolver/TwoStageFFTConvolver.cpp \
	../../modules/FFTConvolver/Utilities.cpp \
	../../modules/r8brain/pffft.cpp \
	../../modules/r8brain/r8bbase.cpp

FILES_UI  = \
	../aidadsp-ui.cpp \
	../../build/Graphics.cpp

ifeq ($(WINDOWS),true)
FILES_UI += ax.rc
endif

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic

DPF_PATH = ../../modules/dpf
DPF_BUILD_DIR = ../../build/dpf
DPF_TARGET_DIR = ../../build/bin
NVG_FONT_TEXTURE_FLAGS = NVG_IMAGE_NEAREST
WINDOWS_ICON_ID = 401
include ../../modules/dpf/Makefile.plugins.mk

# ---------------------------------------------------------------------------------------------------------------------
# Tweak build flags

BUILD_CXX_FLAGS += -std=gnu++17

BUILD_CXX_FLAGS += -DRTNEURAL_DEFAULT_ALIGNMENT=16

BUILD_CXX_FLAGS += -I..
BUILD_CXX_FLAGS += -I../../build
BUILD_CXX_FLAGS += -I../../modules/FFTConvolver
BUILD_CXX_FLAGS += -I../../modules/dr_libs
BUILD_CXX_FLAGS += -I../../modules/r8brain
BUILD_CXX_FLAGS += -I../../modules/rtneural

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for emscripten

ifeq ($(WASM),true)

LINK_FLAGS += -O3
LINK_FLAGS += -sALLOW_MEMORY_GROWTH
LINK_FLAGS += -sINITIAL_MEMORY=64Mb
LINK_FLAGS += -sLZ4=1
LINK_FLAGS += --shell-file=../../utils/ax.html

endif

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for Windows

ifeq ($(WINDOWS),true)

WINDRES ?= $(subst gcc,windres,$(CC))

LINK_FLAGS += -Wl,-subsystem,windows

$(BUILD_DIR)/ax.rc.o: ax.rc ../../utils/ax.ico
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling ax.rc"
	$(SILENT)$(WINDRES) $< -O coff -o $@

endif

# ---------------------------------------------------------------------------------------------------------------------
# Standalone build only

all: jack

# ---------------------------------------------------------------------------------------------------------------------
