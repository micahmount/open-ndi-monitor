#include "audio_ring_buffer.h"
#include <algorithm>

AudioRingBuffer::AudioRingBuffer(size_t capacity_frames, int channels)
    : capacity_(capacity_frames), channels_(channels) {
    buffer_ = new float[capacity_frames * channels];
    write_pos_.store(0);
    read_pos_.store(0);
}

AudioRingBuffer::~AudioRingBuffer() {
    delete[] buffer_;
}

size_t AudioRingBuffer::push(const float* data, size_t frames) {
    size_t write = write_pos_.load(std::memory_order_relaxed);
    size_t read  = read_pos_.load(std::memory_order_acquire);

    size_t used = (write >= read) ? (write - read) : (capacity_ - read + write);
    size_t free = capacity_ - used;
    size_t to_write = std::min(frames, free);

    if (to_write == 0) return 0;

    size_t w = write;
    int ch = channels_;
    for (size_t i = 0; i < to_write; i++) {
        size_t idx = (w % capacity_) * ch;
        for (int c = 0; c < ch; c++) {
            buffer_[idx + c] = data[i * ch + c];
        }
        w++;
    }

    write_pos_.store(w, std::memory_order_release);
    return to_write;
}

size_t AudioRingBuffer::pop(float* data, size_t frames) {
    size_t write = write_pos_.load(std::memory_order_acquire);
    size_t read  = read_pos_.load(std::memory_order_relaxed);

    size_t available = (write >= read) ? (write - read) : (capacity_ - read + write);
    size_t to_read = std::min(frames, available);

    if (to_read == 0) return 0;

    size_t r = read;
    int ch = channels_;
    for (size_t i = 0; i < to_read; i++) {
        size_t idx = (r % capacity_) * ch;
        for (int c = 0; c < ch; c++) {
            data[i * ch + c] = buffer_[idx + c];
        }
        r++;
    }

    read_pos_.store(r, std::memory_order_release);
    return to_read;
}

size_t AudioRingBuffer::available() const {
    size_t write = write_pos_.load(std::memory_order_acquire);
    size_t read  = read_pos_.load(std::memory_order_relaxed);
    return (write >= read) ? (write - read) : (capacity_ - read + write);
}

size_t AudioRingBuffer::free_space() const {
    return capacity_ - available();
}
