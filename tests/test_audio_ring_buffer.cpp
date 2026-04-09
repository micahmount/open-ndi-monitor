#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include "audio_ring_buffer.h"

TEST(AudioRingBufferTest, Constructor) {
    AudioRingBuffer buf(1024, 2);
    EXPECT_EQ(buf.channels(), 2);
    EXPECT_EQ(buf.available(), 0);
    EXPECT_EQ(buf.free_space(), 1024);
}

TEST(AudioRingBufferTest, PushPopBasic) {
    AudioRingBuffer buf(1024, 2);
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    
    size_t written = buf.push(data.data(), 2);
    EXPECT_EQ(written, 2);
    EXPECT_EQ(buf.available(), 2);
    
    std::vector<float> out(4);
    size_t read = buf.pop(out.data(), 2);
    EXPECT_EQ(read, 2);
    EXPECT_FLOAT_EQ(out[0], 1.0f);
    EXPECT_FLOAT_EQ(out[1], 2.0f);
    EXPECT_FLOAT_EQ(out[2], 3.0f); // unchanged
    EXPECT_FLOAT_EQ(out[3], 4.0f);
}

TEST(AudioRingBufferTest, FullBuffer) {
    AudioRingBuffer buf(4, 1);
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    
    size_t written = buf.push(data.data(), 5);
    EXPECT_EQ(written, 4);
    EXPECT_EQ(buf.free_space(), 0);
    
    written = buf.push(data.data(), 1);
    EXPECT_EQ(written, 0);
}

TEST(AudioRingBufferTest, EmptyBuffer) {
    AudioRingBuffer buf(4, 1);
    std::vector<float> out(4);
    
    size_t read = buf.pop(out.data(), 2);
    EXPECT_EQ(read, 0);
}

TEST(AudioRingBufferTest, WrapAround) {
    AudioRingBuffer buf(4, 1);
    std::vector<float> data(8);
    for (int i = 0; i < 8; ++i) data[i] = static_cast<float>(i + 1);
    
    buf.push(data.data(), 4); // fill
    std::vector<float> out(2);
    buf.pop(out.data(), 2);   // free 2
    
    // Now 2 available, 2 free. Push 4 should only write 2.
    size_t written = buf.push(data.data() + 4, 4);
    EXPECT_EQ(written, 2);
    
    std::vector<float> result(4);
    size_t read = buf.pop(result.data(), 4);
    EXPECT_EQ(read, 4);
    EXPECT_FLOAT_EQ(result[0], 3.0f); // was 1,2 then overwritten with 5,6
    EXPECT_FLOAT_EQ(result[3], 6.0f);
}

TEST(AudioRingBufferTest, MultiChannel) {
    AudioRingBuffer buf(1024, 6);
    std::vector<float> data(12); // 2 frames * 6 channels
    
    buf.push(data.data(), 2);
    EXPECT_EQ(buf.available(), 2);
    
    std::vector<float> out(12);
    size_t read = buf.pop(out.data(), 2);
    EXPECT_EQ(read, 2);
}

TEST(AudioRingBufferTest, FreeSpace) {
    AudioRingBuffer buf(100, 2);
    EXPECT_EQ(buf.free_space(), 100);
    
    std::vector<float> data(40);
    buf.push(data.data(), 20);
    EXPECT_EQ(buf.free_space(), 80);
    
    std::vector<float> out(20);
    buf.pop(out.data(), 10);
    EXPECT_EQ(buf.free_space(), 90);
}

TEST(AudioRingBufferTest, EdgeCaseZeroCapacity) {
    AudioRingBuffer buf(0, 2);
    EXPECT_EQ(buf.channels(), 2);
    EXPECT_EQ(buf.free_space(), 0);
}

TEST(AudioRingBufferTest, EdgeCaseZeroChannels) {
    AudioRingBuffer buf(1024, 0);
    EXPECT_EQ(buf.channels(), 1);
    std::vector<float> data(10);
    size_t written = buf.push(data.data(), 5);
    EXPECT_EQ(written, 5);
}

TEST(AudioRingBufferTest, EdgeCasePushMoreThanCapacity) {
    AudioRingBuffer buf(2, 1);
    std::vector<float> data(100, 1.0f);
    size_t written = buf.push(data.data(), 100);
    EXPECT_EQ(written, 2);
}

TEST(AudioRingBufferTest, EdgeCasePopMoreThanAvailable) {
    AudioRingBuffer buf(10, 1);
    std::vector<float> data(5, 1.0f);
    buf.push(data.data(), 5);
    
    std::vector<float> out(100, 0.0f);
    size_t read = buf.pop(out.data(), 100);
    EXPECT_EQ(read, 5);
}