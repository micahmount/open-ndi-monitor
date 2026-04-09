#include <gtest/gtest.h>
#include <vector>
#include "color_convert.h"

TEST(ColorConvertTest, SolidColorWhite) {
    int width = 2, height = 2;
    std::vector<uint8_t> src(width * 2 * height); // UYVY: 2 bytes per pixel
    // U=128, Y=255 (white), V=128
    src[0] = 128; src[1] = 255; src[2] = 128; src[3] = 255;
    src[4] = 128; src[5] = 255; src[6] = 128; src[7] = 255;
    
    std::vector<uint8_t> dst(width * height * 3);
    uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height);
    
    // White: B=255, G=255, R=255
    for (int i = 0; i < width * height * 3; i += 3) {
        EXPECT_GE(dst[i], 240);   // B
        EXPECT_GE(dst[i+1], 240); // G
        EXPECT_GE(dst[i+2], 240); // R
    }
}

TEST(ColorConvertTest, SolidColorBlack) {
    int width = 2, height = 2;
    std::vector<uint8_t> src(width * 2 * height);
    // U=128, Y=16 (black), V=128
    src[0] = 128; src[1] = 16; src[2] = 128; src[3] = 16;
    src[4] = 128; src[5] = 16; src[6] = 128; src[7] = 16;
    
    std::vector<uint8_t> dst(width * height * 3);
    uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height);
    
    // Black: B≈0, G≈0, R≈0
    for (int i = 0; i < width * height * 3; i += 3) {
        EXPECT_LE(dst[i], 20);   // B
        EXPECT_LE(dst[i+1], 20); // G
        EXPECT_LE(dst[i+2], 20); // R
    }
}

TEST(ColorConvertTest, SolidColorRed) {
    int width = 2, height = 2;
    std::vector<uint8_t> src(width * 2 * height);
    // Red: Y=81 (mid-gray), U=90, V=240 (max red)
    src[0] = 90; src[1] = 81; src[2] = 240; src[3] = 81;
    src[4] = 90; src[5] = 81; src[6] = 240; src[7] = 81;
    
    std::vector<uint8_t> dst(width * height * 3);
    uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height);
    
    // Red: B low, G moderate, R high
    EXPECT_LE(dst[0], 80);   // B
    EXPECT_GE(dst[2], 200); // R
}

TEST(ColorConvertTest, SolidColorBlue) {
    int width = 2, height = 2;
    std::vector<uint8_t> src(width * 2 * height);
    // Blue: Y=81 (mid-gray), U=240 (max blue), V=0
    src[0] = 240; src[1] = 81; src[2] = 0; src[3] = 81;
    src[4] = 240; src[5] = 81; src[6] = 0; src[7] = 81;
    
    std::vector<uint8_t> dst(width * height * 3);
    uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height);
    
    // Blue: B high, G moderate, R low
    EXPECT_GE(dst[0], 200);  // B
    EXPECT_LE(dst[2], 80);   // R
}

TEST(ColorConvertTest, SolidColorGreen) {
    int width = 2, height = 2;
    std::vector<uint8_t> src(width * 2 * height);
    // U=0, Y=150, V=0 -> green (actually more like yellow in YUV)
    // Proper green: U=0, V=0 at Y=~100
    src[0] = 0; src[1] = 100; src[2] = 0; src[3] = 100;
    src[4] = 0; src[5] = 100; src[6] = 0; src[7] = 100;
    
    std::vector<uint8_t> dst(width * height * 3);
    uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height);
    
    // Green: B low, G high, R low
    EXPECT_LE(dst[0], 50);   // B
    EXPECT_GE(dst[1], 150);  // G
    EXPECT_LE(dst[2], 50);   // R
}

TEST(ColorConvertTest, PitchHandling) {
    int width = 4, height = 2;
    int pitch = width + 2; // extra bytes between rows
    std::vector<uint8_t> src(pitch * height);
    // Fill with white (U=128, Y=255, V=128) - need 8 bytes per row
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; col += 2) {
            src[row * pitch + col * 2 + 0] = 128;     // U
            src[row * pitch + col * 2 + 1] = 255;     // Y0
            src[row * pitch + col * 2 + 2] = 128;     // V
            src[row * pitch + col * 2 + 3] = 255;     // Y1
        }
    }
    
    std::vector<uint8_t> dst(width * height * 3);
    uyvy_to_bgr24(src.data(), pitch, dst.data(), width, height);
    
    // Should read from correct row offsets
    for (int i = 0; i < width * height; ++i) {
        EXPECT_GE(dst[i*3], 200); // white
    }
}

TEST(ColorConvertTest, EdgeCaseWidthOne) {
    int width = 1, height = 1;
    std::vector<uint8_t> src(width * 2 * height);
    src[0] = 128; src[1] = 255; src[2] = 128; src[3] = 255;
    
    std::vector<uint8_t> dst(width * height * 3);
    EXPECT_NO_THROW(uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height));
}

TEST(ColorConvertTest, EdgeCaseOddWidth) {
    int width = 3, height = 1;
    std::vector<uint8_t> src(width * 2 * height);
    for (int i = 0; i < 6; i++) src[i] = 128;
    
    std::vector<uint8_t> dst(width * height * 3);
    EXPECT_NO_THROW(uyvy_to_bgr24(src.data(), width * 2, dst.data(), width, height));
}

TEST(ColorConvertTest, EdgeCaseZeroWidth) {
    int width = 0, height = 1;
    std::vector<uint8_t> src(10);
    std::vector<uint8_t> dst(10);
    EXPECT_NO_THROW(uyvy_to_bgr24(src.data(), 10, dst.data(), width, height));
}

TEST(ColorConvertTest, EdgeCaseNegativeWidth) {
    int width = -1, height = 1;
    std::vector<uint8_t> src(10);
    std::vector<uint8_t> dst(10);
    EXPECT_NO_THROW(uyvy_to_bgr24(src.data(), 10, dst.data(), width, height));
}

TEST(ColorConvertTest, EdgeCasePitchMismatch) {
    int width = 4, height = 1;
    int pitch = 20; // much larger than needed
    std::vector<uint8_t> src(pitch);
    src[0] = 128; src[1] = 255; src[2] = 128; src[3] = 255;
    src[4] = 128; src[5] = 255; src[6] = 128; src[7] = 255;
    
    std::vector<uint8_t> dst(width * height * 3);
    EXPECT_NO_THROW(uyvy_to_bgr24(src.data(), pitch, dst.data(), width, height));
}