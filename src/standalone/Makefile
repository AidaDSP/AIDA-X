#!/usr/bin/make -f
# Makefile for AIDA-X #
# ------------------- #
# Created by falkTX
#

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = AIDA-X

ifeq ($(NOSIMD),true)
NAME = AIDA-X-NOSIMD
endif

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic (part 1)

export DGL_NAMESPACE = AidaDGL
export NVG_FONT_TEXTURE_FLAGS = NVG_IMAGE_NEAREST
export WASM_EXCEPTIONS = true
export WINDOWS_ICON_ID = 401
export CXXFLAGS += -DPUGL_WASM_NO_KEYBOARD_INPUT -DPUGL_WASM_NO_MOUSEWHEEL_INPUT
include ../../modules/dpf/Makefile.base.mk

ifeq ($(NOSIMD),true)
ifneq (,$(findstring -msse,$(CXXFLAGS)))
$(error NOSIMD build requested but -msse compiler flag is present in CXXFLAGS)
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = \
	aidadsp-plugin.cpp \
	model-activate.cpp \
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

ifeq ($(WASM),true)
APP_EXT = .js
EXTRA_DEPENDENCIES = ../../resources/.stamp_downloaded
endif

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

ifeq ($(MOD_BUILD),true)
BUILD_CXX_FLAGS += -DMOD_BUILD
endif

ifeq ($(NOSIMD),true)
BUILD_CXX_FLAGS += -DNOSIMD -DPFFFT_SIMD_DISABLE
endif

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for generating needed files

$(BUILD_DIR)/Files.cpp.o: $(BUILD_DIR)/Files.cpp

$(BUILD_DIR)/Files.cpp: $(BUILD_DIR)/Files.hpp

$(BUILD_DIR)/Graphics.cpp.o: $(BUILD_DIR)/Graphics.cpp

$(BUILD_DIR)/Graphics.cpp: $(BUILD_DIR)/Graphics.hpp

$(BUILD_DIR)/aidadsp-plugin.cpp.o: $(BUILD_DIR)/Files.hpp

$(BUILD_DIR)/aidadsp-ui.cpp.o: $(BUILD_DIR)/Graphics.hpp

$(BUILD_DIR)/Files.hpp: $(wildcard ../../files/*.*)
	-@mkdir -p $(BUILD_DIR)
	$(DPF_PATH)/utils/res2c.py Files ../../files $(BUILD_DIR)

$(BUILD_DIR)/Graphics.hpp: $(wildcard ../../graphics/png/*.*)
	-@mkdir -p $(BUILD_DIR)
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
LINK_FLAGS += --preload-file=../../resources@/resources
LINK_FLAGS += --use-preload-cache
LINK_FLAGS += --use-preload-plugins

WASM_RESOURCES = $(TARGET_DIR)/AIDA-X.html

$(TARGET_DIR)/AIDA-X.html: ../../utils/ax.html
	-@mkdir -p $(shell dirname $@)
	cp $< $@

../../resources/.stamp_downloaded:
	mkdir -p ../../resources && \
	wget aida.kx.studio/resources.tar.xz -O ../../resources/resources.tar.xz && \
	tar -xvf ../../resources/resources.tar.xz -C ../../resources/ && \
	rm ../../resources/resources.tar.xz

endif

# ---------------------------------------------------------------------------------------------------------------------
# Extra rules for Windows

ifeq ($(WINDOWS),true)

WINDRES ?= $(subst gcc,windres,$(CC))

$(BUILD_DIR)/ax.rc.o: ax.rc ../../utils/ax.ico
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling ax.rc"
	$(SILENT)$(WINDRES) $< -O coff -o $@

endif

# ---------------------------------------------------------------------------------------------------------------------
# Standalone build only

all: jack $(WASM_RESOURCES)

# ---------------------------------------------------------------------------------------------------------------------
