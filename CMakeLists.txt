cmake_minimum_required(VERSION 3.15)
project(aidadsp-loader VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(PREFER_RTNEURAL_XSIMD TRUE)
else()
  set(PREFER_RTNEURAL_XSIMD FALSE)
endif()

set(RTNEURAL_XSIMD ${PREFER_RTNEURAL_XSIMD} CACHE BOOL "Use RTNeural with this backend")
message("RTNEURAL_XSIMD in ${CMAKE_PROJECT_NAME} = ${RTNEURAL_XSIMD}")

add_subdirectory(modules/dpf)
add_subdirectory(modules/rtneural)

find_package(Threads REQUIRED)
set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

dpf_add_plugin(Aida-X
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

target_compile_definitions(dgl-opengl PUBLIC
  NVG_FONT_TEXTURE_FLAGS=NVG_IMAGE_NEAREST
)

target_include_directories(Aida-X PUBLIC
  src
  src/plugin
  modules/dr_libs
  modules/FFTConvolver
  modules/r8brain
  modules/rtneural
  ${CMAKE_BINARY_DIR}
)

target_link_libraries(Aida-X PUBLIC RTNeural ${CMAKE_THREAD_LIBS_INIT})

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
  COMMAND ${CMAKE_SOURCE_DIR}/modules/dpf/utils/res2c.py Graphics ${CMAKE_SOURCE_DIR}/graphics ${CMAKE_BINARY_DIR}
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/graphics
  OUTPUT Graphics.cpp
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
