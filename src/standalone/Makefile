#!/usr/bin/make -f
# Makefile for AIDA-X #
# ------------------- #
# Created by falkTX
#

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = AIDA-X

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic (part 1)

export NVG_FONT_TEXTURE_FLAGS = NVG_IMAGE_NEAREST
export WASM_EXCEPTIONS = true
export WINDOWS_ICON_ID = 401
include ../../modules/dpf/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = \
	aidadsp-plugin.cpp \
	3rd-party.cpp \
	AudioFFT.cpp \
	Biquad.cpp \
	FFTConvolver.cpp \
	Files.cpp \
	TwoStageFFTConvolver.cpp \
	Utilities.cpp \
	pffft.cpp \
	r8bbase.cpp

FILES_UI  = \
	aidadsp-ui.cpp \
	Graphics.cpp \
	Blendish.cpp

ifeq ($(WINDOWS),true)
FILES_UI += ax.rc
endif

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic (part 2)

DPF_PATH = ../../modules/dpf
DPF_BUILD_DIR = ../../build
DPF_TARGET_DIR = ../../build/bin
include ../../modules/dpf/Makefile.plugins.mk

# ---------------------------------------------------------------------------------------------------------------------
# Tweak build flags

BUILD_CXX_FLAGS += -std=gnu++17

# BUILD_CXX_FLAGS += -DBLENDISH_NEW_TLW
BUILD_CXX_FLAGS += -DRTNEURAL_DEFAULT_ALIGNMENT=16

BUILD_CXX_FLAGS += -I..
BUILD_CXX_FLAGS += -I../../build
BUILD_CXX_FLAGS += -I../../modules/dpf-widgets/opengl
BUILD_CXX_FLAGS += -I../../modules/FFTConvolver
BUILD_CXX_FLAGS += -I../../modules/dr_libs
BUILD_CXX_FLAGS += -I../../modules/r8brain
BUILD_CXX_FLAGS += -I../../modules/rtneural
# BUILD_CXX_FLAGS += -I../../modules/rtneural/RTNeural

BUILD_CXX_FLAGS += -DRTNEURAL_USE_EIGEN=1
BUILD_CXX_FLAGS += -I../../modules/rtneural/modules/Eigen

# BUILD_CXX_FLAGS += -DRTNEURAL_USE_XSIMD=1
# BUILD_CXX_FLAGS += -I../../modules/rtneural/modules/xsimd/include

ifeq ($(CPU_I386),true)
# needed for enabling SSE in pffft
BUILD_CXX_FLAGS += -Di386
endif

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for generating needed files

$(BUILD_DIR)/Files.cpp.o: $(BUILD_DIR)/Files.cpp

$(BUILD_DIR)/Graphics.cpp.o: $(BUILD_DIR)/Graphics.cpp

$(BUILD_DIR)/Files.cpp: $(wildcard ../../files/*.*)
	$(DPF_PATH)/utils/res2c.py Files ../../files $(BUILD_DIR)

$(BUILD_DIR)/Graphics.cpp: $(wildcard ../../graphics/png/*.*)
	$(DPF_PATH)/utils/res2c.py Graphics ../../graphics/png $(BUILD_DIR)

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for macOS app bundle

$(TARGET_DIR)/AIDA-X.app/Contents/Info.plist: ../../utils/Info.plist $(TARGET_DIR)/AIDA-X.app/Contents/Resources/ax.icns
	-@mkdir -p $(shell dirname $@)
	cp $< $@

$(TARGET_DIR)/AIDA-X.app/Contents/Resources/ax.icns: ../../utils/ax.icns
	-@mkdir -p $(shell dirname $@)
	cp $< $@

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for emscripten

ifeq ($(WASM),true)

BUILD_CXX_FLAGS += -DEIGEN_DONT_VECTORIZE=1

LINK_FLAGS += -O3
LINK_FLAGS += -sALLOW_MEMORY_GROWTH
LINK_FLAGS += -sINITIAL_MEMORY=64Mb
LINK_FLAGS += -sLZ4=1
LINK_FLAGS += --shell-file=../../utils/ax.html
LINK_FLAGS += --preload-file=../../resources@/resources

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
