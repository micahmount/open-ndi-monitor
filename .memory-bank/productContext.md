# Product Context

## Why This Project Exists
Open-NDI-Monitor is designed to provide a simple, lightweight way to display NDI video sources on Linux systems without requiring full studio software.

## Problems It Solves
1. **No native Linux NDI viewers**: Most NDI tools are Windows/macOS focused
2. **Complex setups**: Studio software is overkill for simple fullscreen display
3. **Headless capability**: Can run without display via config for automation

## User Experience Goals
- Automatic source discovery on startup
- Configurable default source via config file
- Clean fullscreen display with minimal UI
- Visual status indicator (connected/disconnected/reconnecting)
- Keyboard-driven (Escape to quit)

## How It Works
1. Load config from `ndi-monitor.conf` or CLI-specified path
2. Discover NDI sources on the network
3. Connect to specified source (or prompt via CLI if not configured)
4. Receive video frames, convert from UYVY to BGR24, display via SDL2
5. Receive audio frames, resample if needed, play via SDL2 audio
6. Handle reconnection automatically on signal loss

## Configuration Keys
- `source_name`: Preferred NDI source name
- `fullscreen`: true/false for fullscreen mode
- `display_index`: Which display to use (0-indexed)