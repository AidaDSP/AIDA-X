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

dpf_add_plugin(aida-x-plugin
  TARGETS clap lv2 vst2 vst3
  #NO_SHARED_RESOURCES
  UI_TYPE opengl
  FILES_DSP
    Files.cpp
    modules/FFTConvolver/AudioFFT.cpp
    modules/FFTConvolver/FFTConvolver.cpp
    modules/FFTConvolver/TwoStageFFTConvolver.cpp
    modules/FFTConvolver/Utilities.cpp
    src/aidadsp-plugin.cpp
    src/Biquad.cpp
    src/3rd-party.cpp
  FILES_UI
    Graphics.cpp
    src/aidadsp-ui.cpp)

dpf_add_plugin(aida-x-standalone
  TARGETS jack
    #NO_SHARED_RESOURCES
  UI_TYPE opengl
  FILES_DSP
    Files.cpp
    modules/FFTConvolver/AudioFFT.cpp
    modules/FFTConvolver/FFTConvolver.cpp
    modules/FFTConvolver/TwoStageFFTConvolver.cpp
    modules/FFTConvolver/Utilities.cpp
    src/aidadsp-plugin.cpp
    src/Biquad.cpp
    src/3rd-party.cpp
  FILES_UI
    Graphics.cpp
    src/aidadsp-ui.cpp)

target_include_directories(aida-x-plugin PUBLIC src src/plugin modules rtneural ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(aida-x-standalone PUBLIC src src/standalone modules rtneural ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(dgl-opengl PUBLIC NVG_FONT_TEXTURE_FLAGS=NVG_IMAGE_NEAREST)

# convert data into code
add_custom_command(
  PRE_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/dpf/utils/res2c.py Files ${CMAKE_SOURCE_DIR}/files ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/files
  OUTPUT Files.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(
  PRE_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/dpf/utils/res2c.py Graphics ${CMAKE_SOURCE_DIR}/graphics ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/graphics
  OUTPUT Graphics.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# use threads
find_package(Threads REQUIRED)
target_link_libraries(aida-x-plugin PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(aida-x-standalone PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# use IPO
set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
