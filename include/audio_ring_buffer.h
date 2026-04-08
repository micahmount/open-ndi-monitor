#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

// Single-producer single-consumer lock-free ring buffer for interleaved float audio.
// Capacity is in sample frames (one frame = one sample per channel).
class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacity_frames, int channels);
    ~AudioRingBuffer();

    // Push frames into the buffer. Returns number of frames actually written.
    // May write fewer than requested if buffer is full.
    size_t push(const float* data, size_t frames);

    // Pop frames from the buffer. Returns number of frames actually read.
    // May read fewer than requested if buffer is empty.
    size_t pop(float* data, size_t frames);

    // Number of frames currently available to read.
    size_t available() const;

    // Number of frames of free space available to write.
    size_t free_space() const;

    int channels() const { return channels_; }

private:
    float* buffer_;
    size_t capacity_;
    int channels_;
    std::atomic<size_t> write_pos_;
    std::atomic<size_t> read_pos_;
};
