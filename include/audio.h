#pragma once

#include "audio_ring_buffer.h"

// Fixed output sample rate (resample all sources to this)
static constexpr int AUDIO_OUTPUT_RATE = 48000;

struct AudioContext;

// Initialize SDL audio subsystem and open output device.
// Returns nullptr on failure.
AudioContext* audio_init();

// Start audio playback. The SDL callback will begin pulling from the ring buffer.
void audio_start(AudioContext* ctx);

// Stop audio playback.
void audio_stop(AudioContext* ctx);

// Get the ring buffer to push audio frames into.
AudioRingBuffer* audio_buffer(AudioContext* ctx);

// Reconfigure for a new source sample rate. Reinitializes the resampler.
void audio_reconfigure(AudioContext* ctx, int source_rate, int channels);

// Tear down and free the audio context.
void audio_close(AudioContext* ctx);
