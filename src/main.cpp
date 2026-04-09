#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <cstdint>
#include <SDL2/SDL.h>
#include <Processing.NDI.Lib.h>
#include "config.h"
#include "ndi_source.h"
#include "display.h"
#include "color_convert.h"
#include "audio.h"

static std::atomic<bool> g_should_close{false};

void print_dependency_error() {
    std::cerr << "Missing required dependencies. Install with:\n";
    std::cerr << "  sudo apt install libsdl2-2.0 libsamplerate0\n";
    std::cerr << "Or if using .deb package: sudo apt install -f\n";
}

bool check_dependencies() {
    void* sdl = SDL_LoadObject("libSDL2-2.0.so.0");
    if (!sdl) {
        std::cerr << "Error: SDL2 not found\n";
        print_dependency_error();
        return false;
    }
    SDL_UnloadObject(sdl);

    void* ndi = SDL_LoadObject("libndi.so.6");
    if (!ndi) {
        std::cerr << "Error: NDI runtime not found\n";
        std::cerr << "Install the .deb package or copy NDI libs to system:\n";
        std::cerr << "  sudo cp /path/to/libndi.so* /usr/lib/x86_64-linux-gnu/\n";
        return false;
    }
    SDL_UnloadObject(ndi);

    return true;
}

// Wrapper that pumps events and checks for close signals
void pump_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT ||
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            g_should_close = true;
        }
    }
}

// Receive frames until connection drops or user quits.
// Returns true if connection was lost (needs reconnect), false if user quit.
bool receive_loop(NDIlib_recv_instance_t recv, DisplayContext* display,
                  AudioContext* audio, std::vector<uint8_t>& bgr_buffer) {
    NDIlib_video_frame_v2_t video_frame;
    NDIlib_audio_frame_v2_t audio_frame;

    int consecutive_timeouts = 0;
    const int max_timeouts = 3;  // 3 * 5s = 15s before declaring connection lost

    while (!g_should_close && !display_should_close(display)) {
        pump_events();

        switch (NDIlib_recv_capture_v2(recv, &video_frame, &audio_frame, nullptr, 5000)) {
            case NDIlib_frame_type_video:
                consecutive_timeouts = 0;
                {
                    int w = video_frame.xres;
                    int h = video_frame.yres;
                    int src_pitch = video_frame.line_stride_in_bytes;

                    // Validate frame parameters to prevent crashes
                    if (w <= 0 || h <= 0 || src_pitch <= 0) {
                        std::cerr << "Invalid video frame: " << w << "x" << h << " pitch=" << src_pitch << "\n";
                        NDIlib_recv_free_video_v2(recv, &video_frame);
                        break;
                    }

                    if (bgr_buffer.size() < static_cast<size_t>(w * h * 3)) {
                        bgr_buffer.resize(w * h * 3);
                    }

                    uyvy_to_bgr24(static_cast<const uint8_t*>(video_frame.p_data),
                                  src_pitch,
                                  bgr_buffer.data(), w, h);

                    update_display(display, bgr_buffer.data(), w, h);
                    display_draw_text(display, "connected", 20, 20);
                    NDIlib_recv_free_video_v2(recv, &video_frame);
                }
                break;

            case NDIlib_frame_type_audio:
                consecutive_timeouts = 0;
                if (audio) {
                    audio_push_frame(audio, &audio_frame);
                }
                NDIlib_recv_free_audio_v2(recv, &audio_frame);
                break;

            case NDIlib_frame_type_error:
                std::cerr << "NDI receive error\n";
                return true;

            default:
                // Timeout - no frame received
                consecutive_timeouts++;
                if (consecutive_timeouts >= max_timeouts) {
                    std::cerr << "Connection lost (no frames for " 
                              << (consecutive_timeouts * 5) << "s)\n";
                    return true;
                }
                break;
        }
    }

    return false;  // User quit, not connection loss
}

// Create a receiver for the given source
NDIlib_recv_instance_t create_receiver(const NdiSourceInfo& source) {
    NDIlib_source_t ndi_source = {0};
    ndi_source.p_ndi_name = source.name.c_str();
    ndi_source.p_url_address = source.url_address.c_str();

    NDIlib_recv_create_v3_t recv_desc = {0};
    recv_desc.source_to_connect_to = ndi_source;
    recv_desc.color_format = NDIlib_recv_color_format_UYVY_BGRA;
    recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;

    return NDIlib_recv_create_v3(&recv_desc);
}

int main(int argc, char* argv[]) {
#ifdef APP_VERSION
    printf("open-ndi-monitor v%s\n", APP_VERSION);
#else
    printf("open-ndi-monitor v0.1.0\n");
#endif

    if (!check_dependencies()) {
        return 1;
    }

    // Load config
    Config cfg;
    std::string config_path = "ndi-monitor.conf";

    // Allow overriding config path via --config flag
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printf("Usage: open-ndi-monitor [--config <path>]\n");
            return 0;
        }
    }

    auto loaded = load_config(config_path);
    if (loaded) {
        cfg = *loaded;
        printf("Loaded config from %s\n", config_path.c_str());
    } else {
        printf("No config found at %s, using defaults\n", config_path.c_str());
    }

    // Discover NDI sources
    printf("Discovering NDI sources...\n");
    auto sources = discover_sources(10000);

    if (sources.empty()) {
        std::cerr << "No NDI sources found on the network\n";
        return 1;
    }

    // Select source
    int source_idx = select_source_cli(sources, cfg.source_name);
    if (source_idx < 0) {
        std::cerr << "No source selected\n";
        return 1;
    }

    NdiSourceInfo selected = sources[source_idx];
    printf("Connecting to: %s (%s)\n", selected.name.c_str(), selected.url_address.c_str());

    // Create display
    DisplayContext* display = create_display(cfg.display_index, selected.name);
    if (!display) {
        std::cerr << "Failed to create display\n";
        return 1;
    }

    // Initialize audio
    AudioContext* audio = audio_init();
    if (audio) {
        printf("Audio initialized\n");
    } else {
        printf("Audio initialization failed, video only\n");
    }

    // Initialize NDI library (once, for the lifetime of the app)
    if (!NDIlib_initialize()) {
        std::cerr << "Failed to initialize NDI library\n";
        destroy_display(display);
        if (audio) audio_close(audio);
        return 1;
    }

    // BGR24 output buffer (allocated once, reused)
    std::vector<uint8_t> bgr_buffer;

    // Reconnection loop
    int reconnect_attempts = 0;
    const int max_reconnect_delay_ms = 30000;  // 30s cap
    const int base_reconnect_delay_ms = 1000;   // 1s start

    while (!g_should_close && !display_should_close(display)) {
        // Create receiver
        NDIlib_recv_instance_t recv = create_receiver(selected);
        if (!recv) {
            std::cerr << "Failed to create NDI receiver\n";
            display_draw_text(display, "no_signal", 20, 20);
        } else {
            // Start audio playback
            if (audio) {
                audio_start(audio);
            }

            printf("Receiving video (press Escape to quit)...\n");
            display_draw_text(display, "connected", 20, 20);

            // Run receive loop - returns true if connection lost
            bool connection_lost = receive_loop(recv, display, audio, bgr_buffer);

            // Stop audio while reconnecting
            if (audio) {
                audio_stop(audio);
            }

            if (!connection_lost) {
                // User quit
                NDIlib_recv_destroy(recv);
                break;
            }

            NDIlib_recv_destroy(recv);
        }

        // Reconnection with exponential backoff
        reconnect_attempts++;
        int delay_ms = std::min(base_reconnect_delay_ms * (1 << (reconnect_attempts - 1)),
                                max_reconnect_delay_ms);

        printf("Reconnecting in %dms (attempt %d)...\n", delay_ms, reconnect_attempts);
        display_draw_text(display, "reconnecting", 20, 20);

        // Pump events during reconnect delay
        auto reconnect_start = std::chrono::steady_clock::now();
        while (!g_should_close && !display_should_close(display)) {
            pump_events();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - reconnect_start).count();
            if (elapsed >= delay_ms) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (g_should_close || display_should_close(display)) {
            break;
        }

        // Try the same URL first, then re-discover if that fails
        printf("Trying to reconnect to %s...\n", selected.name.c_str());
        auto test_sources = discover_sources(5000);

        bool found = false;
        for (const auto& src : test_sources) {
            if (src.name == selected.name) {
                found = true;
                selected.url_address = src.url_address;  // URL may have changed
                printf("Found source at %s\n", src.url_address.c_str());
                break;
            }
        }

        if (!found) {
            printf("Source not found at original URL, showing source list...\n");
            if (test_sources.empty()) {
                printf("No NDI sources on network, waiting...\n");
                continue;  // Will retry with same source info
            }

            int new_idx = select_source_cli(test_sources, cfg.source_name);
            if (new_idx < 0) {
                printf("No source selected, will retry original...\n");
                continue;
            }
            selected = test_sources[new_idx];
            reconnect_attempts = 0;  // Reset backoff for new source
        } else {
            // Found the same source, reset backoff
            reconnect_attempts = 0;
        }
    }

    printf("Shutting down...\n");

    // Cleanup
    if (audio) {
        audio_stop(audio);
    }
    NDIlib_destroy();
    destroy_display(display);
    if (audio) {
        audio_close(audio);
    }

    return 0;
}
