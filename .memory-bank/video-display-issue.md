# open-ndi-monitor Development Notes

## Current Issue: Video Display Black Screen

### Symptoms
- Camera NDI streams show "Video frame" in logs and "Rendered frame" is logged
- But the window displays **black** (no video visible)
- OBS NDI streams don't receive any video frames at all (only audio)
- Connection is established (connection count = 1)
- Green status indicator appears

### Root Cause Analysis

**Camera (CAM1, CAM2, CAM3):**
- Frames ARE being received successfully
- Format: `FourCC=0x59565955` (UYVY)
- Resolution: 1920x1080
- "Rendered frame" logged
- But display is BLACK

**OBS NDI on Mac:**
- Audio frames arrive (logged as "audio=1")
- Video frames NEVER arrive
- This may be an OBS-side issue (NDI output not enabled for video)

### What We've Tried

1. ✅ Added BGRA/BGRX format support (in `color_convert.cpp`)
2. ✅ Added UYVY format support
3. ✅ Added I420/NV12 format support
4. ✅ Changed to `NDIlib_recv_color_format_fastest`
5. ✅ Changed to `NDIlib_recv_color_format_BGRX_BGRA`
6. ✅ Added connection wait before receive loop
7. ✅ Added extensive logging to file
8. ✅ Disabled audio to test if it was interfering
9. ✅ Initialized buffer with gray color for debugging

### Current State (v0.1.13)
- Color format: BGRX_BGRA (forced)
- Bandwidth: highest
- allow_video_fields: true
- Buffer initialized with gray (128,128,128) on resize
- Audio: disabled by default (use `--enable-audio` to enable)

### Key Findings

1. **UYVY format (0x59565955)** is being received from cameras
2. `bgra_to_bgr24()` is being called (not the UYVY converter)
3. This means NDI SDK is converting to BGRA, then we're converting to BGR24
4. The conversion appears to work (frames logged as rendered)
5. But display shows black - likely a rendering/display issue

### Next Steps

1. **Check SDL texture creation** - is the texture being created correctly?
2. **Check display.cpp** - verify update_display is working
3. **Test with known-good source** - create a simple test pattern
4. **Try direct BGRA display** without conversion
5. **Check display resolution** vs source resolution mismatch

### Files Involved

- `src/main.cpp` - NDI receive loop, format handling
- `src/display.cpp` - SDL2 window, texture, rendering
- `src/color_convert.cpp` - UYVY, BGRA, I420, NV12 to BGR24
- `include/color_convert.h` - Conversion function declarations

### Debug Commands

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

### Version History (Debug Versions)

- v0.1.15: Set allow_video_fields=false to request full frames
- v0.1.14: Fallback to software renderer if accelerated fails
- v0.1.13: Added BGRA/BGRX format support (in `color_convert.cpp`)
- v0.1.12: Added --enable-audio flag, connection wait
- v0.1.11: Metadata parsing for FourCC/resolution
- v0.1.10: I420/NV12 support, fastest color format
- v0.1.9: Connection count logging
- v0.1.8: Metadata capture
- v0.1.7: File logging
