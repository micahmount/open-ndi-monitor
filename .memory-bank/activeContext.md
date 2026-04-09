# Active Context

## Current Work Focus
No active development - project is feature-complete.

## Recent Changes
- Project initialized with full NDI video/audio display capabilities
- Reconnection logic implemented with exponential backoff
- Status indicator added (green=connected, red=no signal, yellow=reconnecting)
- Refactored project structure: renamed `out/` to `build/`

## Next Steps
- None - project meets requirements

## Active Decisions and Considerations
- Config file location: `ndi-monitor.conf` in working directory or via `--config` flag
- Status indicator: 32x32 colored square in top-left corner of window