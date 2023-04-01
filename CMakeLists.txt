cmake_minimum_required(VERSION 3.15)
project(aidadsp-loader VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

# if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(amd64|AMD64|i.86|x64|X64|x86|x86_64|X86)$")
#   set(PREFER_RTNEURAL_XSIMD TRUE)
# else()
set(PREFER_RTNEURAL_XSIMD FALSE)
# endif()

set(RTNEURAL_XSIMD ${PREFER_RTNEURAL_XSIMD} CACHE BOOL "Use RTNeural with this backend")
message("RTNEURAL_XSIMD in ${CMAKE_PROJECT_NAME} = ${RTNEURAL_XSIMD}, using processor type ${CMAKE_SYSTEM_PROCESSOR} and system name ${CMAKE_SYSTEM_NAME}")

add_subdirectory(modules/dpf)
add_subdirectory(modules/rtneural)

find_package(Threads REQUIRED)
set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

dpf_add_plugin(AIDA-X
  TARGETS clap lv2 vst2 vst3
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
set_target_properties(AIDA-X-Standalone-jack PROPERTIES WIN32 TRUE)
target_sources(AIDA-X-Standalone-jack PUBLIC utils/ax.rc)
endif()

# set up windows installer
if(WIN32)
add_custom_command(
  POST_BUILD
  COMMAND utils/windows-installer.sh "${CMAKE_PROJECT_VERSION}"
  DEPENDS AIDA-X-clap AIDA-X-lv2 AIDA-X-lv2-ui AIDA-X-vst2 AIDA-X-vst3 AIDA-X-Standalone-jack
  OUTPUT "${CMAKE_SOURCE_DIR}/utils/inno/AIDA-X-win64-${CMAKE_PROJECT_VERSION}-installer.exe"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_custom_target(AIDA-X-Installer ALL
  DEPENDS "${CMAKE_SOURCE_DIR}/utils/inno/AIDA-X-win64-${CMAKE_PROJECT_VERSION}-installer.exe"
)
endif()
