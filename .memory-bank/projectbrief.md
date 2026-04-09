# Project Brief

## Overview
- **Name**: open-ndi-monitor
- **Type**: C++17 desktop application (single binary)
- **Purpose**: Display NDI video sources fullscreen via SDL2
- **Platform**: Linux

## Core Requirements
1. Fullscreen NDI video display
2. Audio playback with SDL2
3. Automatic source discovery and selection
4. Reconnection on signal loss
5. Configurable via INI file

## Build System
- CMake 3.16+
- Single binary output: `build/bin/open-ndi-monitor`

## Dependencies
- NDI SDK (Linux)
- SDL2
- libsamplerate (audio resampling)

## Status
- Feature-complete for basic NDI display and audio
- Testing infrastructure in place (Google Test)