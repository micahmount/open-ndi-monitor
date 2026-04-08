#include <SDL.h>
#include <samplerate.h>
#include <Processing.NDI.Lib.h>
#include <Processing.NDI.utilities.h>
#include <iostream>
#include <vector>
#include <cstring>
#include "audio.h"

struct AudioContext {
    SDL_AudioDeviceID device = 0;
    AudioRingBuffer* buffer = nullptr;
    SRC_STATE* resampler = nullptr;
    int source_rate = 0;
    int channels = 0;
    std::vector<float> interleaved_temp;
    std::vector<float> resample_temp;
    std::vector<float> resample_input;
};

// SDL audio callback - pulls from ring buffer, outputs silence on underrun
static void sdl_audio_callback(void* userdata, Uint8* stream, int len) {
    auto* ctx = static_cast<AudioContext*>(userdata);
    float* out = reinterpret_cast<float*>(stream);
    size_t frames_needed = len / (sizeof(float) * ctx->channels);

    size_t got = ctx->buffer->pop(out, frames_needed);

    // Fill remaining with silence if underrun
    if (got < frames_needed) {
        std::memset(out + got * ctx->channels, 0,
                    (frames_needed - got) * ctx->channels * sizeof(float));
    }
}

AudioContext* audio_init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_InitSubSystem(AUDIO) failed: " << SDL_GetError() << "\n";
        return nullptr;
    }

    auto* ctx = new AudioContext();

    // Ring buffer: ~0.5s at 48kHz (will be sized when we know channels)
    // Default to stereo, reconfigure when we get actual audio
    ctx->buffer = new AudioRingBuffer(AUDIO_OUTPUT_RATE / 2, 2);

    SDL_AudioSpec spec = {};
    spec.freq = AUDIO_OUTPUT_RATE;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;  // Will be updated on reconfigure
    spec.samples = 1024;
    spec.callback = sdl_audio_callback;
    spec.userdata = ctx;

    ctx->device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (ctx->device == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << "\n";
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        delete ctx->buffer;
        delete ctx;
        return nullptr;
    }

    return ctx;
}

void audio_start(AudioContext* ctx) {
    if (ctx && ctx->device) {
        SDL_PauseAudioDevice(ctx->device, 0);
    }
}

void audio_stop(AudioContext* ctx) {
    if (ctx && ctx->device) {
        SDL_PauseAudioDevice(ctx->device, 1);
    }
}

AudioRingBuffer* audio_buffer(AudioContext* ctx) {
    return ctx ? ctx->buffer : nullptr;
}

void audio_reconfigure(AudioContext* ctx, int source_rate, int channels) {
    if (!ctx) return;

    // If nothing changed, skip
    if (ctx->source_rate == source_rate && ctx->channels == channels) {
        return;
    }

    ctx->source_rate = source_rate;
    ctx->channels = channels;

    // Recreate ring buffer with correct channel count
    // ~0.5s of audio at output rate
    size_t capacity_frames = AUDIO_OUTPUT_RATE / 2;
    delete ctx->buffer;
    ctx->buffer = new AudioRingBuffer(capacity_frames, channels);

    // Recreate resampler
    if (ctx->resampler) {
        src_delete(ctx->resampler);
        ctx->resampler = nullptr;
    }

    if (source_rate != AUDIO_OUTPUT_RATE) {
        int error;
        ctx->resampler = src_new(SRC_SINC_MEDIUM_QUALITY, channels, &error);
        if (!ctx->resampler) {
            std::cerr << "Failed to create resampler: " << src_strerror(error) << "\n";
        }
    }

    // Resize temp buffers
    // interleaved_temp: enough for max NDI audio frame (~4800 samples at 48kHz)
    ctx->interleaved_temp.resize(48000 * channels);
    ctx->resample_temp.resize(48000 * channels);
    ctx->resample_input.resize(48000 * channels);

    std::cerr << "Audio reconfigured: " << source_rate << "Hz/" << channels
              << "ch -> " << AUDIO_OUTPUT_RATE << "Hz/" << channels << "ch\n";
}

void audio_close(AudioContext* ctx) {
    if (!ctx) return;

    if (ctx->device) {
        SDL_CloseAudioDevice(ctx->device);
    }
    if (ctx->resampler) {
        src_delete(ctx->resampler);
    }
    if (ctx->buffer) {
        delete ctx->buffer;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    delete ctx;
}

// Push an NDI audio frame into the audio system.
// Converts planar FLTP -> interleaved float, resamples if needed, pushes to ring buffer.
void audio_push_frame(AudioContext* ctx, const NDIlib_audio_frame_v2_t* frame) {
    if (!ctx || !frame || frame->no_samples <= 0) return;

    int channels = frame->no_channels;
    int samples = frame->no_samples;
    int source_rate = frame->sample_rate;

    // Reconfigure if source rate or channels changed
    audio_reconfigure(ctx, source_rate, channels);

    // Convert planar FLTP -> interleaved float using NDI utility
    NDIlib_audio_frame_interleaved_32f_t interleaved = {};
    interleaved.sample_rate = source_rate;
    interleaved.no_channels = channels;
    interleaved.no_samples = samples;
    interleaved.p_data = ctx->interleaved_temp.data();

    NDIlib_util_audio_to_interleaved_32f_v2(frame, &interleaved);

    if (source_rate == AUDIO_OUTPUT_RATE) {
        // No resampling needed
        ctx->buffer->push(interleaved.p_data, samples);
    } else if (ctx->resampler) {
        // Resample to output rate
        SRC_DATA src_data = {};
        src_data.data_in = interleaved.p_data;
        src_data.input_frames = samples;
        src_data.src_ratio = static_cast<double>(AUDIO_OUTPUT_RATE) / source_rate;
        src_data.data_out = ctx->resample_temp.data();
        src_data.output_frames = static_cast<long>(samples * src_data.src_ratio) + 64;

        int error = src_process(ctx->resampler, &src_data);
        if (error == 0) {
            ctx->buffer->push(src_data.data_out, src_data.output_frames_gen);
        } else {
            std::cerr << "Resampler error: " << src_strerror(error) << "\n";
        }
    }
}
