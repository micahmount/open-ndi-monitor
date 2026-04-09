# AGENTS.md — open-ndi-monitor

## Project
- C++17 single-binary app (CMake 3.16+)
- Displays NDI video sources fullscreen via SDL2
- **Unit tests via Google Test** — run with `ctest` or individual test binaries

## Build
```
cmake -B build && cmake --build build
```
Binary lands in `build/bin/open-ndi-monitor`

## Test
```
cd build && ctest --output-on-failure
# or run individually:
./build/bin/test_config
./build/bin/test_color_convert
./build/bin/test_audio_ring_buffer
```

## Dependencies (must be installed before building)
- **NDI SDK**: extracted to `resources/ndi-sdk/NDI SDK for Linux` (included in repo)
- **SDL2**: `libsdl2-dev`
- **libsamplerate**: `libsamplerate0-dev`

## Architecture
- `src/main.cpp` — entrypoint: loads config, discovers NDI sources, runs receive loop with reconnection
- `src/ndi_source.cpp` — NDI source discovery and CLI selection
- `src/display.cpp` — SDL2 fullscreen window, texture upload, event pumping, status indicator
- `src/config.cpp` — minimal INI parser (`key=value`, `#` comments)
- `src/color_convert.cpp` — UYVY→BGR24 conversion (BT.601)
- `src/audio.cpp` — SDL2 audio + libsamplerate resampling
- `src/audio_ring_buffer.cpp` — SPSC lock-free ring buffer
- `include/*.h` — matching headers for each module

## Runtime
- Config file: `ndi-monitor.conf` (INI-style). Override with `--config <path>`
- Keys: `source_name`, `fullscreen`, `display_index`
- Press **Escape** or close window to quit
- Status indicator: colored square in top-left (green=connected, red=no signal, yellow=reconnecting)

## Reconnection
- 3 consecutive 5s timeouts = connection lost
- Exponential backoff: 1s → 2s → 4s → ... → 30s cap
- Reconnection flow: try same source → re-discover → CLI picker

## Directories
- `build/` — CMake build output (gitignored)
- `memory-bank/` — project documentation
- `resources/` — NDI SDK installer tarball
