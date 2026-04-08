cmake_minimum_required(VERSION 3.16)
project(open-ndi-monitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# NDI SDK
set(NDI_SDK_PATH "${CMAKE_SOURCE_DIR}/ndi-sdk/NDI SDK for Linux")
find_path(NDI_INCLUDE_DIR NAMES Processing.NDI.Lib.h PATHS "${NDI_SDK_PATH}/include" NO_DEFAULT_PATH)
find_library(NDI_LIBRARY NAMES ndi PATHS "${NDI_SDK_PATH}/lib/x86_64-linux-gnu" NO_DEFAULT_PATH)

if(NOT NDI_INCLUDE_DIR OR NOT NDI_LIBRARY)
    message(FATAL_ERROR "NDI SDK not found. Set -DNDI_SDK_PATH=/path/to/sdk")
endif()

# SDL2 - optional for testing
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 sdl2)
endif()
if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found - main app will not be built")
endif()

# libsamplerate - optional for testing
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(SAMPLERATE samplerate)
endif()
if(NOT SAMPLERATE_FOUND)
    message(STATUS "libsamplerate not found - main app will not be built")
endif()

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON)
FetchContent_MakeAvailable(googletest)
enable_testing()

# Build main app only if all dependencies available
if(SDL2_FOUND AND SAMPLERATE_FOUND)
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
else()
    message(STATUS "Skipping main app build - missing dependencies")
endif()

# Tests
add_executable(test_config tests/test_config.cpp src/config.cpp)
target_include_directories(test_config PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_config PRIVATE GTest::gtest_main)

add_executable(test_color_convert tests/test_color_convert.cpp src/color_convert.cpp)
target_include_directories(test_color_convert PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_color_convert PRIVATE GTest::gtest_main)

add_executable(test_audio_ring_buffer tests/test_audio_ring_buffer.cpp src/audio_ring_buffer.cpp)
target_include_directories(test_audio_ring_buffer PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_audio_ring_buffer PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_config test_color_convert test_audio_ring_buffer)

# Install - only if main app was built
if(TARGET open-ndi-monitor)
    install(TARGETS open-ndi-monitor DESTINATION bin)
    install(DIRECTORY "${NDI_SDK_PATH}/lib/x86_64-linux-gnu/" DESTINATION lib/open-ndi-monitor FILES_MATCHING PATTERN "libndi.so*")
    install(FILES "${CMAKE_SOURCE_DIR}/open-ndi-monitor.service" DESTINATION lib/systemd/system)
endif()

# CPack Debian
set(CPACK_PACKAGE_NAME "open-ndi-monitor")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NDI video monitor")
set(CPACK_PACKAGE_MAINTAINER "Micah")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0, libsamplerate0")
set(CPACK_DEBIAN_PACKAGE_REPLACES "ndi-sdk")
set(CPACK_DEBIAN_PACKAGE_SECTION "video")
include(CPack)
