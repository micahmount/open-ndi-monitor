#pragma once

#include <cstdint>

// Convert UYVY (YUV 4:2:2 interleaved) to BGR24.
void uyvy_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height);

// Convert BGRA to BGR24 (drops alpha).
void bgra_to_bgr24(const uint8_t* src, int src_pitch,
                    uint8_t* dst, int width, int height);

// Convert BGRX to BGR24 (drops unused byte).
void bgrx_to_bgr24(const uint8_t* src, int src_pitch,
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

// Convert YV12 (YVU 4:2:0 planar) to BGR24.
void yv12_to_bgr24(const uint8_t* src_y, int y_pitch,
                    const uint8_t* src_v, int v_pitch,
                    const uint8_t* src_u, int u_pitch,
                    uint8_t* dst, int width, int height);

// Convert RGBX (RGB packed) to BGR24.
void rgbx_to_bgr24(const uint8_t* src, int src_pitch,
                    uint8_t* dst, int width, int height);

// Convert RGBA to BGR24 (drops alpha).
void rgba_to_bgr24(const uint8_t* src, int src_pitch,
                    uint8_t* dst, int width, int height);

// Convert UYVA (UYVY + alpha) to BGR24.
void uyva_to_bgr24(const uint8_t* src, int src_pitch,
                    uint8_t* dst, int width, int height);

// Convert P216 (16-bit YUV 4:2:2 semi-planar) to BGR24.
void p216_to_bgr24(const uint8_t* src_y, int y_pitch,
                   const uint8_t* src_uv, int uv_pitch,
                   uint8_t* dst, int width, int height);

// Convert PA16 (16-bit YUV 4:2:2:4 with alpha) to BGR24.
void pa16_to_bgr24(const uint8_t* src_y, int y_pitch,
                    const uint8_t* src_uv, int uv_pitch,
                    const uint8_t* src_a, int a_pitch,
                    uint8_t* dst, int width, int height);
