#pragma once

#include <cstdint>

// Convert UYVY (YUV 4:2:2 interleaved) to BGR24.
void uyvy_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height);

// Convert BGRA to BGR24 (just drops alpha channel).
void bgra_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height);

// Convert I420 (YUV 4:2:0 planar) to BGR24.
void i420_to_bgr24(const uint8_t* src_y, int y_pitch,
                    const uint8_t* src_u, int u_pitch,
                    const uint8_t* src_v, int v_pitch,
                    uint8_t* dst, int width, int height);

// Convert NV12 (YUV 4:2:0 semi-planar) to BGR24.
void nv12_to_bgr24(const uint8_t* src_y, int y_pitch,
                    const uint8_t* src_uv, int uv_pitch,
                    uint8_t* dst, int width, int height);
