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

add_subdirectory(dpf)
add_subdirectory(rtneural)

dpf_add_plugin(aidadsp-loader
  TARGETS clap lv2 vst2 vst3
  #NO_SHARED_RESOURCES
  UI_TYPE opengl
  FILES_DSP
    modules/FFTConvolver/AudioFFT.cpp
    modules/FFTConvolver/FFTConvolver.cpp
    modules/FFTConvolver/TwoStageFFTConvolver.cpp
    modules/FFTConvolver/Utilities.cpp
    src/aidadsp-plugin.cpp
    src/3rd-party.cpp
  FILES_UI
    Artwork.cpp
    src/aidadsp-ui.cpp)

target_compile_definitions(dgl-opengl PUBLIC NVG_FONT_TEXTURE_FLAGS=NVG_IMAGE_NEAREST)
target_include_directories(aidadsp-loader PUBLIC src modules rtneural ${CMAKE_CURRENT_BINARY_DIR})

# convert images into code
add_custom_command(
  PRE_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/dpf/utils/res2c.py Artwork ${CMAKE_SOURCE_DIR}/graphics ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/graphics
  OUTPUT Artwork.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# use threads
find_package(Threads)
target_link_libraries(aidadsp-loader PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# needed for enabling SSE in pffft under 32bit systems
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86|X86)$")
  target_compile_definitions(aidadsp-loader PUBLIC i386=1)
endif()
