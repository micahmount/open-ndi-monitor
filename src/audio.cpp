#include <SDL.h>
#include <samplerate.h>
#include <Processing.NDI.Lib.h>
#include <Processing.NDI.utilities.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include "audio.h"
#include "audio_ring_buffer.h"

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
    if (!ctx || ctx->channels == 0) return;
    
    int16_t* out = reinterpret_cast<int16_t*>(stream);
    size_t frames_needed = len / (sizeof(int16_t) * ctx->channels);

    std::vector<float> float_buffer(frames_needed * ctx->channels);
    size_t got = ctx->buffer->pop(float_buffer.data(), frames_needed);

    // Convert float to int16
    for (size_t i = 0; i < got * ctx->channels; i++) {
        out[i] = static_cast<int16_t>(float_buffer[i] * 32767.0f);
    }

    // Fill remaining with silence if underrun
    if (got < frames_needed) {
        std::memset(out + got * ctx->channels, 0,
                    (frames_needed - got) * ctx->channels * sizeof(int16_t));
    }
}

AudioContext* audio_init() {
    return audio_init_with_device("");
}

AudioContext* audio_init_with_device(const std::string& device_name) {
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
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;  // Will be updated on reconfigure
    spec.samples = 1024;
    spec.callback = sdl_audio_callback;
    spec.userdata = ctx;

    // Open specified device or default if empty
    const char* device = device_name.empty() ? nullptr : device_name.c_str();
    ctx->device = SDL_OpenAudioDevice(device, 0, &spec, nullptr, 0);
    if (ctx->device == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << "\n";
        if (!device_name.empty()) {
            // Try default device as fallback
            std::cerr << "Falling back to default audio device\n";
            ctx->device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
            if (ctx->device == 0) {
                std::cerr << "SDL_OpenAudioDevice fallback also failed: " << SDL_GetError() << "\n";
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
                delete ctx->buffer;
                delete ctx;
                return nullptr;
            }
        } else {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            delete ctx->buffer;
            delete ctx;
            return nullptr;
        }
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

    // Guard against invalid values
    if (source_rate <= 0 || channels <= 0) {
        std::cerr << "Invalid audio params: " << source_rate << "Hz, " << channels << "ch\n";
        return;
    }

    // If nothing changed, skip
    if (ctx->source_rate == source_rate && ctx->channels == channels) {
        return;
    }

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

    // Validate before reconfigure to prevent division by zero
    if (source_rate <= 0 || channels <= 0 || samples <= 0) {
        std::cerr << "Invalid audio frame: rate=" << source_rate 
                  << "Hz, channels=" << channels << ", samples=" << samples << "\n";
        return;
    }

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

void audio_list_devices() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_InitSubSystem(AUDIO) failed: " << SDL_GetError() << "\n";
        return;
    }

    int num_devices = SDL_GetNumAudioDevices(0);
    printf("Available audio output devices:\n");
    for (int i = 0; i < num_devices; i++) {
        printf("  [%d] %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }
    if (num_devices == 0) {
        printf("  No audio devices found\n");
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

std::string audio_find_hdmi_device() {
    // Use ALSA to find HDMI device
    FILE* fp = popen("aplay -l 2>/dev/null | grep -i hdmi | head -1", "r");
    if (!fp) return "";
    
    char buffer[256];
    std::string result;
    if (fgets(buffer, sizeof(buffer), fp)) {
        result = buffer;
    }
    pclose(fp);
    
    if (result.empty()) return "";
    
    // Parse "card 0: PCH [HDA Intel PCH], device 7: HDMI 1 [HDMI 1]"
    // Extract card and device numbers
    size_t card_pos = result.find("card ");
    size_t device_pos = result.find("device ");
    
    if (card_pos == std::string::npos || device_pos == std::string::npos) return "";
    
    card_pos += 5;
    size_t card_end = result.find(":", card_pos);
    if (card_end == std::string::npos) return "";
    
    device_pos += 8;
    size_t device_end = result.find(":", device_pos);
    if (device_end == std::string::npos) device_end = result.length();
    
    std::string card = result.substr(card_pos, card_end - card_pos);
    std::string device = result.substr(device_pos, device_end - device_pos);
    
    // Trim whitespace
    card.erase(remove_if(card.begin(), card.end(), ::isspace), card.end());
    device.erase(remove_if(device.begin(), device.end(), ::isspace), device.end());
    
    if (card.empty() || device.empty()) return "";
    
    return "hw:" + card + "," + device;
}
