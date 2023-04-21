
# Config

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Define static libs

add_library(dgl STATIC IMPORTED)
set_property(TARGET dgl PROPERTY IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libdgl-opengl.a")

add_library(sAidaX STATIC IMPORTED)
set_property(TARGET sAidaX PROPERTY IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/bin/AIDA-X.a")

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

juce_add_plugin(AIDA-X-JUCE
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

set_target_properties(AIDA-X-JUCE_AU
    PROPERTIES
        OUTPUT_NAME "AIDA-X"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_sources(AIDA-X-JUCE
    PRIVATE
        JuceWrapper.cpp)

target_include_directories(AIDA-X-JUCE
    PRIVATE
        ..
        ../plugin
        ../../modules/dpf/distrho)

target_compile_definitions(AIDA-X-JUCE
    PUBLIC
        DISTRHO_NAMESPACE=AidaDISTRHO
        DGL_NAMESPACE=AidaDGL
        JucePlugin_ForcedStereo=1
        JucePlugin_PreferredChannelConfigurations=2,2
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

target_link_libraries(AIDA-X-JUCE
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
