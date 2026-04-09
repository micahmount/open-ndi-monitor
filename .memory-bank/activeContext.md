# Active Context

## Current Work Focus
**Debugging: Video display shows black screen despite frames being received**

### Symptoms
- Camera NDI streams: "Video frame #X" and "Rendered frame" logged, but window shows black
- OBS NDI on Mac: Audio frames arrive, video frames never arrive
- Connection established (connection count = 1)
- Green status indicator appears

### Root Cause (Partial)
- Camera streams send UYVY format (FourCC=0x59565955)
- NDI SDK converts to BGRA internally
- We're converting BGRA → BGR24 successfully
- "Rendered frame" logged every frame
- But SDL2 display is BLACK

### Key Findings
- v0.1.12: Audio disabled → Camera video frames started arriving!
- v0.1.13: BGRX_BGRA format, buffer initialized with gray
- OBS PGM Test: Works for audio only (video frames never received - OBS-side issue?)
- Camera sources: Frames received and "rendered" but display is black

## Recent Changes
- v0.1.13: Force BGRX_BGRA format, init buffer with gray (128,128,128)
- v0.1.12: Added --enable-audio flag, wait for connection before receive loop
- v0.1.11: Metadata parsing for FourCC/resolution extraction
- v0.1.10: I420/NV12 format support, fastest color format
- v0.1.9: Connection count logging
- v0.1.8: NDI metadata capture
- v0.1.7: File logging to ~/.open-ndi-monitor.log

## Next Steps
1. Check display.cpp for rendering issues
2. Verify SDL texture upload is correct
3. Test with direct BGRA display (no conversion)
4. Check if display resolution matches source resolution

## Debug Commands
```bash
# Install latest
sudo dpkg -i open-ndi-monitor_0.1.13_amd64.deb

# Run without audio
open-ndi-monitor

# Run with audio
open-ndi-monitor --enable-audio

# Check log
cat ~/.open-ndi-monitor.log
```

## Key Files
- `src/main.cpp` - NDI receive loop, format handling, conversion
- `src/display.cpp` - SDL2 window, texture, rendering
- `src/color_convert.cpp` - UYVY, BGRA, I420, NV12 to BGR24
- `.memory-bank/video-display-issue.md` - Detailed debugging notes
