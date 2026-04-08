cmake_minimum_required(VERSION 3.16)
project(open-ndi-monitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# NDI SDK
set(NDI_SDK_PATH "/opt/ndi/sdk" CACHE PATH "Path to NDI SDK root")
find_path(NDI_INCLUDE_DIR NAMES Processing.NDI.Lib.h PATHS "${NDI_SDK_PATH}/include" NO_DEFAULT_PATH)
find_library(NDI_LIBRARY NAMES ndi PATHS "${NDI_SDK_PATH}/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)

if(NOT NDI_INCLUDE_DIR OR NOT NDI_LIBRARY)
    message(FATAL_ERROR "NDI SDK not found. Set -DNDI_SDK_PATH=/path/to/sdk")
endif()

# SDL2
find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found. Install libsdl2-dev.")
endif()

# libsamplerate
find_package(PkgConfig REQUIRED)
pkg_check_modules(SAMPLERATE REQUIRED samplerate)

# Sources
add_executable(open-ndi-monitor
    src/main.cpp
    src/config.cpp
    src/ndi_source.cpp
    src/display.cpp
    src/color_convert.cpp
    src/audio_ring_buffer.cpp
    src/audio.cpp
)

target_include_directories(open-ndi-monitor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${NDI_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SAMPLERATE_INCLUDE_DIRS}
)

target_link_libraries(open-ndi-monitor PRIVATE
    ${NDI_LIBRARY}
    ${SDL2_LIBRARIES}
    ${SAMPLERATE_LIBRARIES}
)

# Install
install(TARGETS open-ndi-monitor DESTINATION bin)
install(DIRECTORY "${NDI_SDK_PATH}/lib/x86_64-linux-gnu/" DESTINATION lib/open-ndi-monitor FILES_MATCHING PATTERN "libndi.so*")
