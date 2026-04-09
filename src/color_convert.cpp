#include "color_convert.h"
#include <algorithm>
#include <cstring>

static inline void yuv_to_bgr(int y, int u, int v, uint8_t* bgr) {
    int c = y - 16;
    int d = u - 128;
    int e = v - 128;

    int b = std::clamp((298 * c + 516 * d + 128) >> 8, 0, 255);
    int g = std::clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255);
    int r = std::clamp((298 * c + 409 * e + 128) >> 8, 0, 255);

    bgr[0] = static_cast<uint8_t>(b);
    bgr[1] = static_cast<uint8_t>(g);
    bgr[2] = static_cast<uint8_t>(r);
}

void uyvy_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height) {
    for (int row = 0; row < height; ++row) {
        const uint8_t* line = src + row * src_pitch;
        uint8_t* out = dst + row * width * 3;

        for (int col = 0; col < width; col += 2) {
            int u  = line[col * 2 + 0];
            int y0 = line[col * 2 + 1];
            int v  = line[col * 2 + 2];
            int y1 = line[col * 2 + 3];

            yuv_to_bgr(y0, u, v, out + col * 3);
            yuv_to_bgr(y1, u, v, out + (col + 1) * 3);
        }
    }
}

void bgra_to_bgr24(const uint8_t* src, int src_pitch,
                   uint8_t* dst, int width, int height) {
    for (int row = 0; row < height; ++row) {
        const uint8_t* line = src + row * src_pitch;
        uint8_t* out = dst + row * width * 3;

        for (int col = 0; col < width; col++) {
            out[col * 3 + 0] = line[col * 4 + 0];  // B
            out[col * 3 + 1] = line[col * 4 + 1];  // G
            out[col * 3 + 2] = line[col * 4 + 2];  // R
        }
    }
}

void i420_to_bgr24(const uint8_t* src_y, int y_pitch,
                   const uint8_t* src_u, int u_pitch,
                   const uint8_t* src_v, int v_pitch,
                   uint8_t* dst, int width, int height) {
    for (int row = 0; row < height; ++row) {
        const uint8_t* y_line = src_y + row * y_pitch;
        const uint8_t* u_line = src_u + (row / 2) * u_pitch;
        const uint8_t* v_line = src_v + (row / 2) * v_pitch;
        uint8_t* out = dst + row * width * 3;

        for (int col = 0; col < width; ++col) {
            int y = y_line[col];
            int u = u_line[col / 2];
            int v = v_line[col / 2];
            yuv_to_bgr(y, u, v, out + col * 3);
        }
    }
}

void nv12_to_bgr24(const uint8_t* src_y, int y_pitch,
                   const uint8_t* src_uv, int uv_pitch,
                   uint8_t* dst, int width, int height) {
    for (int row = 0; row < height; ++row) {
        const uint8_t* y_line = src_y + row * y_pitch;
        const uint8_t* uv_line = src_uv + (row / 2) * uv_pitch;
        uint8_t* out = dst + row * width * 3;

        for (int col = 0; col < width; ++col) {
            int y = y_line[col];
            int u = uv_line[(col / 2) * 2 + 0];
            int v = uv_line[(col / 2) * 2 + 1];
            yuv_to_bgr(y, u, v, out + col * 3);
        }
    }
}
