# open-ndi-monitor

A lightweight, performant Linux application that displays NDI video sources fullscreen on a dedicated monitor. Designed for studio environments where you need a simple "set and forget" display endpoint.

## Features

- **Fullscreen NDI display** — Renders any NDI source to a borderless fullscreen window
- **Audio output** — Plays NDI audio with automatic sample rate conversion to 48kHz
- **Multi-monitor support** — Configure which display to use via config file
- **Auto-reconnection** — Automatically reconnects when the source drops (exponential backoff)
- **Status indicator** — Visual feedback showing connection state (green/yellow/red)
- **Simple configuration** — INI-style config file, no complex setup

## Requirements

- Linux (x86_64)
- NDI SDK v6 (included in `resources/`)
- SDL2
- libsamplerate

## Building

```bash
# Install dependencies
sudo apt install libsdl2-dev libsamplerate0-dev

# Extract NDI SDK (if not already installed at /opt/ndi/sdk)
tar -xzf resources/Install_NDI_SDK_v6_Linux.tar.gz -C /opt

# Build
cmake -B out && cmake --build out
```

The binary will be at `out/bin/open-ndi-monitor`.

## Usage

```bash
# Run (will prompt for source selection)
./out/bin/open-ndi-monitor

# Or with a config file
./out/bin/open-ndi-monitor --config /path/to/config.conf
```

### Config File

Create a `ndi-monitor.conf` file:

```ini
# NDI source name (empty = prompt for selection)
source_name=My NDI Source

# Fullscreen mode
fullscreen=true

# Display index (0 = primary)
display_index=0
```

### Controls

- **Escape** — Quit
- **Close window** — Quit

## Architecture

```
src/main.cpp          — Entry point, connection management, reconnection loop
src/ndi_source.cpp    — NDI source discovery and selection
src/display.cpp       — SDL2 fullscreen window and rendering
src/color_convert.cpp — UYVY → BGR24 color conversion
src/audio.cpp         — SDL2 audio output with libsamplerate
src/audio_ring_buffer.cpp — Lock-free SPSC ring buffer for audio
src/config.cpp        — INI configuration parser
```

## License

See LICENSE file (pending).

## Acknowledgments

- [NDI SDK](https://www.ndi.video/) — NewTek's NDI technology
- [SDL2](https://libsdl.org/) — Simple DirectMedia Layer
- [libsamplerate](https://www.mega-nerd.com/libsamplerate/) — Secret Rabbit Code
