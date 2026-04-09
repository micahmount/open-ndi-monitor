# System Patterns

## Architecture Overview
Single-threaded main loop with callback-based NDI reception. SDL2 handles both video display and audio playback in the main thread.

```
main.cpp → ndi_source.cpp → display.cpp / audio.cpp
           ↓
      color_convert.cpp (video)
      audio_ring_buffer.cpp (audio)
```

## Key Components

### ndi_source.cpp
- NDI discovery via `NDIlib_find_create` / `NDIlib_find_wait_for_sources`
- Source selection (config preference or CLI picker)
- Connection with `NDIlib_recv_create_v3`

### display.cpp
- SDL2 window creation (windowed or fullscreen)
- Texture creation for video frames
- `SDL_UpdateTexture` for frame upload
- Status indicator rendering (32x32 colored rect)

### color_convert.cpp
- UYVY to BGR24 conversion
- BT.601 color matrix
- Inline SIMD-friendly implementation

### audio.cpp
- SDL2 audio device open callback
- libsamplerate for sample rate conversion
- SPSC ring buffer for lock-free frame delivery

### audio_ring_buffer.cpp
- Single-producer-single-consumer lock-free ring buffer
- Atomic head/tail indices
- Fixed-size buffer (currently 48000 samples)

## Design Patterns
- **Callback-driven**: NDI provides frames via callback
- **Ring buffer**: Lock-free audio queue between threads
- **INI parsing**: Simple key=value config (config.cpp)
- **State machine**: Connection states (connected/reconnecting/disconnected)

## Component Relationships
- `main.cpp` owns the application lifecycle
- `display.cpp` and `audio.cpp` are independent (SDL2 manages threading)
- NDI receiver runs on separate thread, main thread pulls frames via `NDIlib_recv_capture`