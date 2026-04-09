cmake_minimum_required(VERSION 3.16)

# Get version from git tag (e.g., v1.2.3 -> 1.2.3)
execute_process(COMMAND git describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "v" "" PROJECT_VERSION "${GIT_TAG}")
if(NOT PROJECT_VERSION)
    set(PROJECT_VERSION "0.0.0")  # Fallback for tarball builds
endif()

project(open-ndi-monitor VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# NDI SDK
set(NDI_SDK_PATH "${CMAKE_SOURCE_DIR}/resources/ndi-sdk/NDI SDK for Linux")
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

    # Set RPATH so the binary can find libndi.so in /usr/lib/open-ndi-monitor
    set_target_properties(open-ndi-monitor PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib/open-ndi-monitor"
        INSTALL_RPATH "/usr/lib/open-ndi-monitor"
    )
else()
    message(STATUS "Skipping main app build - missing dependencies")
endif()

# Tests - conditionally build if GTest target exists (FetchContent or find_package)
if(TARGET GTest::gtest_main)
    add_executable(test_config tests/test_config.cpp src/config.cpp)
    target_include_directories(test_config PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test_config PRIVATE GTest::gtest_main)
    add_test(NAME test_config COMMAND test_config)

    add_executable(test_color_convert tests/test_color_convert.cpp src/color_convert.cpp)
    target_include_directories(test_color_convert PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test_color_convert PRIVATE GTest::gtest_main)
    add_test(NAME test_color_convert COMMAND test_color_convert)

    add_executable(test_audio_ring_buffer tests/test_audio_ring_buffer.cpp src/audio_ring_buffer.cpp)
    target_include_directories(test_audio_ring_buffer PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test_audio_ring_buffer PRIVATE GTest::gtest_main)
    add_test(NAME test_audio_ring_buffer COMMAND test_audio_ring_buffer)
endif()

# Install - only if main app was built
if(TARGET open-ndi-monitor)
    # Define version at compile time for the binary
    target_compile_definitions(open-ndi-monitor PRIVATE APP_VERSION="${PROJECT_VERSION}")

    # Install main binary
    install(TARGETS open-ndi-monitor DESTINATION bin)

    # Install NDI library
    install(DIRECTORY "${NDI_SDK_PATH}/lib/x86_64-linux-gnu/" DESTINATION lib/x86_64-linux-gnu FILES_MATCHING PATTERN "libndi.so*")

    # Install systemd service
    install(FILES "${CMAKE_SOURCE_DIR}/open-ndi-monitor.service" DESTINATION lib/systemd/system)
endif()
