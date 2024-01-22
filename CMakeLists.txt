cmake_minimum_required(VERSION 3.15)
project(AIDA-X VERSION 1.1.0)

set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISTRHO_NAMESPACE=AidaDISTRHO -DDGL_NAMESPACE=AidaDGL")
if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

# if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(amd64|AMD64|i.86|x64|X64|x86|x86_64|X86)$")
#   set(PREFER_RTNEURAL_XSIMD TRUE)
# else()
set(PREFER_RTNEURAL_XSIMD FALSE)
# endif()
set(RTNEURAL_XSIMD ${PREFER_RTNEURAL_XSIMD} CACHE BOOL "Use RTNeural with this backend")
message("RTNEURAL_XSIMD in ${CMAKE_PROJECT_NAME} = ${RTNEURAL_XSIMD}, using processor type ${CMAKE_SYSTEM_PROCESSOR} and system name ${CMAKE_SYSTEM_NAME}")

if(APPLE)
set(PREFER_JUCE_EXTRA TRUE)
else()
set(PREFER_JUCE_EXTRA FALSE)
endif()
set(JUCE_EXTRA ${PREFER_JUCE_EXTRA} CACHE BOOL "Use JUCE for extra plugin formats")

add_subdirectory(modules/dpf)
add_subdirectory(modules/rtneural)

find_package(Threads REQUIRED)
if(NOT WIN32)
  set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(TARGETS clap lv2 vst2 vst3)

# hacky hack part1
if(JUCE_EXTRA)
set(TARGETS ${TARGETS} static)
endif()

dpf_add_plugin(AIDA-X
  TARGETS ${TARGETS}
  #NO_SHARED_RESOURCES
  UI_TYPE opengl
  FILES_DSP
    Files.cpp
    modules/FFTConvolver/AudioFFT.cpp
    modules/FFTConvolver/FFTConvolver.cpp
    modules/FFTConvolver/TwoStageFFTConvolver.cpp
    modules/FFTConvolver/Utilities.cpp
    modules/r8brain/pffft.cpp
    modules/r8brain/r8bbase.cpp
    src/aidadsp-plugin.cpp
    src/model-activate.cpp
    src/Biquad.cpp
    src/3rd-party.cpp
  FILES_UI
    Graphics.cpp
    src/aidadsp-ui.cpp)

dpf_add_plugin(AIDA-X-Standalone
  TARGETS jack
  #NO_SHARED_RESOURCES
  UI_TYPE opengl
  FILES_DSP
    Files.cpp
    modules/FFTConvolver/AudioFFT.cpp
    modules/FFTConvolver/FFTConvolver.cpp
    modules/FFTConvolver/TwoStageFFTConvolver.cpp
    modules/FFTConvolver/Utilities.cpp
    modules/r8brain/pffft.cpp
    modules/r8brain/r8bbase.cpp
    src/aidadsp-plugin.cpp
    src/model-activate.cpp
    src/Biquad.cpp
    src/3rd-party.cpp
  FILES_UI
    Graphics.cpp
    modules/dpf-widgets/opengl/Blendish.cpp
    src/aidadsp-ui.cpp)

target_compile_definitions(dgl-opengl PUBLIC
  DGL_WINDOWS_ICON_ID=401
  NVG_FONT_TEXTURE_FLAGS=NVG_IMAGE_NEAREST
)

# hacky hack part 2
if(JUCE_EXTRA)
include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/DISTRHO/JUCE.git
  GIT_TAG v6.1.6-distrho
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)
add_subdirectory(src/jucewrapper)
if (APPLE)
add_dependencies(AIDA-X-JUCE_AU AIDA-X-static dgl-opengl)
target_compile_definitions(AIDA-X PUBLIC DISTRHO_PLUGIN_TARGET_STATIC_NAME="AudioUnit")
else()
add_dependencies(AIDA-X-JUCE_Standalone AIDA-X-static dgl-opengl)
target_compile_definitions(AIDA-X PUBLIC DISTRHO_PLUGIN_TARGET_STATIC_NAME="Standalone")
endif()
endif()

target_include_directories(AIDA-X PUBLIC
  src
  src/plugin
  modules/dr_libs
  modules/FFTConvolver
  modules/r8brain
  modules/rtneural
  ${CMAKE_BINARY_DIR}
)

target_include_directories(AIDA-X-Standalone PUBLIC
  src
  src/standalone
  modules/dpf-widgets/opengl
  modules/dr_libs
  modules/FFTConvolver
  modules/r8brain
  modules/rtneural
  ${CMAKE_BINARY_DIR}
)

# needed for enabling SSE in pffft
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86|X86)$")
  target_compile_definitions(AIDA-X PUBLIC i386)
  target_compile_definitions(AIDA-X-Standalone PUBLIC i386)
endif()

# needed for emscripten
if(EMSCRIPTEN)
  target_compile_definitions(RTNeural PUBLIC EIGEN_DONT_VECTORIZE=1)
endif()

# needed for RISC-V
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
  set(CMAKE_OPTIONAL_LIBATOMIC "-latomic")
endif()

target_link_libraries(AIDA-X PUBLIC RTNeural ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_OPTIONAL_LIBATOMIC})
target_link_libraries(AIDA-X-Standalone PUBLIC RTNeural ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_OPTIONAL_LIBATOMIC})

# convert data into code
add_custom_command(
  PRE_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/modules/dpf/utils/res2c.py Files ${CMAKE_SOURCE_DIR}/files ${CMAKE_BINARY_DIR}
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/files
  OUTPUT Files.cpp
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(
  PRE_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/modules/dpf/utils/res2c.py Graphics ${CMAKE_SOURCE_DIR}/graphics/png ${CMAKE_BINARY_DIR}
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/graphics
  OUTPUT Graphics.cpp
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# set up extra standalone details
set_target_properties(AIDA-X-Standalone-jack PROPERTIES OUTPUT_NAME "AIDA-X")

if(APPLE)
set_target_properties(AIDA-X-Standalone-jack PROPERTIES
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/utils/Info.plist"
)
add_custom_command(
  POST_BUILD
  COMMAND cmake -E copy "${CMAKE_SOURCE_DIR}/utils/ax.icns" "${CMAKE_BINARY_DIR}/bin/AIDA-X.app/Contents/Resources/ax.icns"
  DEPENDS AIDA-X-Standalone-jack
  OUTPUT "${CMAKE_BINARY_DIR}/bin/AIDA-X.app/Contents/Resources/ax.icns"
)
add_custom_target(AIDA-X-App ALL
  DEPENDS "${CMAKE_BINARY_DIR}/bin/AIDA-X.app/Contents/Resources/ax.icns"
)
elseif(WIN32)
target_sources(AIDA-X-Standalone-jack PUBLIC utils/ax.rc)
endif()

# set up windows installer
if(WIN32 AND MINGW)
add_custom_command(
  POST_BUILD
  COMMAND utils/windows-installer.sh "${CMAKE_BINARY_DIR}" "${CMAKE_PROJECT_VERSION}"
  DEPENDS AIDA-X-clap AIDA-X-lv2 AIDA-X-lv2-ui AIDA-X-vst2 AIDA-X-vst3 AIDA-X-Standalone-jack
  OUTPUT "${CMAKE_SOURCE_DIR}/utils/inno/AIDA-X-${CMAKE_PROJECT_VERSION}-win64-installer.exe"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_custom_target(AIDA-X-Installer ALL
  DEPENDS "${CMAKE_SOURCE_DIR}/utils/inno/AIDA-X-${CMAKE_PROJECT_VERSION}-win64-installer.exe"
)
endif()
