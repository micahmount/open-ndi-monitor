#pragma once

#include <cstdint>

// Convert UYVY (YUV 4:2:2 interleaved) to BGR24.
// src: UYVY data, src_pitch: bytes per row in source
// dst: BGR24 output buffer (must be width*height*3 bytes)
// width/height: frame dimensions (width must be even)
void uyvy_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height);

// Convert BGRA to BGR24 (just drops alpha channel).
// src: BGRA data, src_pitch: bytes per row in source
// dst: BGR24 output buffer (must be width*height*3 bytes)
void bgra_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height);
