cmake_minimum_required(VERSION 3.15)
project(AIDA-X VERSION 0.1.0)

add_subdirectory(JUCE)

# Config

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Define static libs

add_library(dgl STATIC IMPORTED)
set_property(TARGET dgl PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/../../modules/dpf/build/libdgl-opengl.a")

add_library(sAidaX STATIC IMPORTED)
set_property(TARGET sAidaX PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/../../build/bin/AIDA-X.a")

# dependencies

if (APPLE)
set(GL_LIBRARIES "-framework OpenGL")
set(PLUGIN_FORMATS AU)
else (APPLE)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GL REQUIRED gl)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XCURSOR REQUIRED xcursor)
pkg_check_modules(XEXT REQUIRED xext)
pkg_check_modules(XRANDR REQUIRED xrandr)
set(STATIC_LIBS_START "-Wl,--whole-archive")
set(STATIC_LIBS_END "-Wl,--no-whole-archive")
set(PLUGIN_FORMATS Standalone VST3)
endif (APPLE)

juce_add_plugin(AIDA-X
    AU_MAIN_TYPE kAudioUnitType_Effect
    COMPANY_COPYRIGHT "GPL-3.0-or-later"
    COMPANY_NAME "AIDA DSP"
    COMPANY_WEBSITE "https://github.com/AidaDSP/AIDA-X"
    DESCRIPTION "AI Crafted Tone"
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    FORMATS ${PLUGIN_FORMATS}
    IS_MIDI_EFFECT FALSE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_CODE AidX
    PLUGIN_MANUFACTURER_CODE Aida
    PRODUCT_NAME "AIDA-X")

target_sources(AIDA-X
    PRIVATE
        JuceWrapper.cpp)

target_include_directories(AIDA-X
    PRIVATE
        ..
        ../plugin
        ../../modules/dpf/distrho)

target_compile_definitions(AIDA-X
    PUBLIC
        DGL_NAMESPACE=AidaDGL
        JucePlugin_PreferredChannelConfigurations=1,1
        JUCE_CHECK_MEMORY_LEAKS=0
        JUCE_DISABLE_NATIVE_FILECHOOSERS=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=0
        JUCE_USE_CURL=0
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_XINERAMA=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_ALSA=0
        JUCE_DIRECTSOUND=0
        JUCE_JACK=0
        JUCE_WASAPI=0
        JUCE_WEB_BROWSER=0)

target_link_libraries(AIDA-X
    PRIVATE
        juce::juce_audio_utils
        ${STATIC_LIBS_START}
        sAidaX
        dgl
        ${STATIC_LIBS_END}
        ${GL_LIBRARIES}
        ${DBUS_LIBRARIES}
        ${X11_LIBRARIES}
        ${XCURSOR_LIBRARIES}
        ${XEXT_LIBRARIES}
        ${XRANDR_LIBRARIES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
