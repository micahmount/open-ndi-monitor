# Tech Context

## Technologies Used

### NDI SDK
- Version: Linux SDK (included in repo at `resources/ndi-sdk/NDI SDK for Linux`)
- Key headers: `Processing.NDI.Lib.h`
- Library: `libndi.so`

### SDL2
- Purpose: Video display and audio playback
- Package: `libsdl2-dev`
- Key functions: `SDL_CreateWindow`, `SDL_CreateRenderer`, `SDL_UpdateTexture`, `SDL_OpenAudio`

### libsamplerate
- Purpose: High-quality audio sample rate conversion
- Package: `libsamplerate0-dev`
- Key functions: `src_new`, `src_process`, `src_delete`

### Google Test
- Purpose: Unit testing
- Fetched via CMake FetchContent (v1.14.0)
- Tests: test_config, test_color_convert, test_audio_ring_buffer

## Development Setup

### Build
```bash
cmake -B build && cmake --build build
```

### Run Tests
```bash
cd build && ctest --output-on-failure
```

### Dependencies (Ubuntu/Debian)
```bash
sudo apt install libsdl2-dev libsamplerate0-dev
```

### NDI SDK Location
Expected at: `resources/ndi-sdk/NDI SDK for Linux`

## Technical Constraints
- C++17 minimum
- CMake 3.16+
- Linux only (NDI SDK for Linux)
- Single binary output

## Dependencies Graph
```
open-ndi-monitor
├── NDI SDK (libndi.so)
├── SDL2 (video + audio)
└── libsamplerate (audio resampling)
    └── libsndfile (libsamplerate dependency)
```