#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <cstdint>
#include <cstdarg>
#include <ctime>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <Processing.NDI.Lib.h>
#include "config.h"
#include "ndi_source.h"
#include "display.h"
#include "color_convert.h"
#include "audio.h"
#include "user_detect.h"

static std::atomic<bool> g_should_close{false};

static std::ofstream g_logfile;

void log(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    std::string timestamp = []{
        time_t now = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }();
    
    std::string line = "[" + timestamp + "] " + buf + "\n";
    std::printf("%s", line.c_str());
    if (g_logfile.is_open()) {
        g_logfile << line << std::flush;
    }
}

void log_close() {
    if (g_logfile.is_open()) {
        g_logfile.close();
    }
}

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
    NDIlib_metadata_frame_t meta_frame;

    int consecutive_timeouts = 0;
    int64_t frame_count = 0;
    int64_t audio_count = 0;

    log("Entering receive loop");

    const int max_timeouts = 3;  // 3 * 5s = 15s before declaring connection lost

    while (!g_should_close && !display_should_close(display)) {
        pump_events();

        int connections = NDIlib_recv_get_no_connections(recv);
        if (consecutive_timeouts == 0 || connections != (consecutive_timeouts > 0 ? 1 : connections)) {
            log("Connection count: %d", connections);
        }

        // Log connection stats every 10 seconds
        if (frame_count > 0 && frame_count % 200 == 0) {
            NDIlib_recv_queue_t queue;
            NDIlib_recv_get_queue(recv, &queue);
            log("Queue depth: video=%d, audio=%d", queue.video_frames, queue.audio_frames);
        }

        switch (NDIlib_recv_capture_v2(recv, &video_frame, &audio_frame, &meta_frame, 5000)) {
            case NDIlib_frame_type_metadata:
                if (meta_frame.p_data) {
                    log("Metadata: %s", meta_frame.p_data);
                    NDIlib_recv_free_metadata(recv, &meta_frame);
                }
                break;
            case NDIlib_frame_type_video:
                consecutive_timeouts = 0;
                frame_count++;
                {
                    int w = video_frame.xres;
                    int h = video_frame.yres;
                    int src_pitch = video_frame.line_stride_in_bytes;

                    log("Video frame #%lld: %dx%d, pitch=%d, FourCC=0x%08X", 
                        (long long)frame_count,
                        w, h, src_pitch, 
                        static_cast<unsigned int>(video_frame.FourCC));

                    // Validate frame parameters
                    if (w <= 0 || h <= 0 || src_pitch <= 0) {
                        log("ERROR: Invalid video frame: %dx%d pitch=%d", w, h, src_pitch);
                        NDIlib_recv_free_video_v2(recv, &video_frame);
                        break;
                    }

                    if (bgr_buffer.size() < static_cast<size_t>(w * h * 3)) {
                        bgr_buffer.resize(w * h * 3);
                    }

                    bool converted = false;

                    if (video_frame.FourCC == NDIlib_FourCC_type_BGRA ||
                        video_frame.FourCC == NDIlib_FourCC_type_BGRX) {
                        bgra_to_bgr24(static_cast<const uint8_t*>(video_frame.p_data),
                                      src_pitch,
                                      bgr_buffer.data(), w, h);
                        converted = true;
                    } else if (video_frame.FourCC == NDIlib_FourCC_type_UYVY) {
                        uyvy_to_bgr24(static_cast<const uint8_t*>(video_frame.p_data),
                                      src_pitch,
                                      bgr_buffer.data(), w, h);
                        converted = true;
                    } else if (video_frame.FourCC == NDIlib_FourCC_type_I420) {
                        const uint8_t* y = static_cast<const uint8_t*>(video_frame.p_data);
                        const uint8_t* u = y + h * src_pitch;
                        const uint8_t* v = u + (h / 2) * (src_pitch / 2);
                        i420_to_bgr24(y, src_pitch, u, src_pitch / 2, v, src_pitch / 2,
                                      bgr_buffer.data(), w, h);
                        converted = true;
                    } else if (video_frame.FourCC == NDIlib_FourCC_type_NV12) {
                        const uint8_t* y = static_cast<const uint8_t*>(video_frame.p_data);
                        const uint8_t* uv = y + h * src_pitch;
                        nv12_to_bgr24(y, src_pitch, uv, src_pitch,
                                      bgr_buffer.data(), w, h);
                        converted = true;
                    } else {
                        log("ERROR: Unsupported video format: FourCC=0x%08X", 
                            static_cast<unsigned int>(video_frame.FourCC));
                    }

                    if (converted) {
                        update_display(display, bgr_buffer.data(), w, h);
                        display_draw_text(display, "connected", 20, 20);
                    }

                    NDIlib_recv_free_video_v2(recv, &video_frame);
                }
                break;

            case NDIlib_frame_type_audio:
                consecutive_timeouts = 0;
                audio_count++;
                if (audio) {
                    audio_push_frame(audio, &audio_frame);
                }
                NDIlib_recv_free_audio_v2(recv, &audio_frame);
                break;

            case NDIlib_frame_type_error:
                log("NDI receive error");
                return true;

            default:
                consecutive_timeouts++;
                if (consecutive_timeouts == 1) {
                    log("Timeout waiting for frames (video=%lld, audio=%lld)...", 
                        (long long)frame_count, (long long)audio_count);
                }
                if (consecutive_timeouts >= max_timeouts) {
                    log("Connection lost: no frames for %ds (video=%lld, audio=%lld)", 
                        consecutive_timeouts * 5, (long long)frame_count, (long long)audio_count);
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
    recv_desc.color_format = NDIlib_recv_color_format_fastest;
    recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
    recv_desc.allow_video_fields = false;
    recv_desc.p_ndi_recv_name = "open-ndi-monitor";

    log("Creating receiver: color_format=fastest, bandwidth=highest");

    return NDIlib_recv_create_v3(&recv_desc);
}

int main(int argc, char* argv[]) {
#ifdef APP_VERSION
    printf("open-ndi-monitor v%s\n", APP_VERSION);
#else
    printf("open-ndi-monitor v0.1.0\n");
#endif

    // Open log file
    const char* home = getenv("HOME");
    std::string log_path = home ? std::string(home) + "/.open-ndi-monitor.log" : "/tmp/open-ndi-monitor.log";
    g_logfile.open(log_path, std::ios::app);
    if (g_logfile.is_open()) {
        printf("Logging to %s\n", log_path.c_str());
        log("=== Application started ===");
    }

    // Detect and switch to the logged-in user (for systemd service running as root)
    auto user = detect_logged_in_user();
    if (user) {
        printf("Detected user: %s (uid=%d)\n", user->username.c_str(), user->uid);
        if (getuid() == 0) {
            if (switch_to_user(*user)) {
                printf("Switched to user %s\n", user->username.c_str());
            } else {
                fprintf(stderr, "Warning: Could not switch to user %s, continuing as root\n", 
                        user->username.c_str());
            }
        }
    }

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
        } else if (std::strcmp(argv[i], "--audio-device") == 0 && i + 1 < argc) {
            cfg.audio_device = argv[++i];
        } else if (std::strcmp(argv[i], "--list-audio-devices") == 0) {
            audio_list_devices();
            return 0;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printf("Usage: open-ndi-monitor [--config <path>] [--audio-device <name>] [--list-audio-devices]\n");
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
    std::string audio_device = cfg.audio_device;
    if (audio_device.empty()) {
        // Try to auto-detect HDMI device
        audio_device = audio_find_hdmi_device();
        if (!audio_device.empty()) {
            printf("Auto-detected HDMI audio device: %s\n", audio_device.c_str());
        }
    }
    AudioContext* audio = audio_init_with_device(audio_device);
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

    const int reconnect_timeout_ms = 10000;  // 10 seconds to try reconnecting

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

            // Query connected source name
            const char* connected_name = nullptr;
            if (NDIlib_recv_get_source_name(recv, &connected_name, 5000) && connected_name) {
                log("Connected to source: %s", connected_name);
            }

            // Run receive loop - returns true if connection lost
            bool connection_lost = receive_loop(recv, display, audio, bgr_buffer);

            // Stop audio
            if (audio) {
                audio_stop(audio);
            }

            NDIlib_recv_destroy(recv);

            if (!connection_lost) {
                // User quit
                break;
            }

            // Connection lost - try to reconnect within timeout
            printf("Connection lost, attempting to reconnect...\n");
            display_draw_text(display, "reconnecting", 20, 20);

            auto reconnect_start = std::chrono::steady_clock::now();
            bool reconnected = false;

            while (!g_should_close && !display_should_close(display)) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - reconnect_start).count();

                if (elapsed >= reconnect_timeout_ms) {
                    break;  // Timeout exceeded
                }

                pump_events();

                // Try to reconnect
                auto test_sources = discover_sources(2000);
                for (const auto& src : test_sources) {
                    if (src.name == selected.name) {
                        selected.url_address = src.url_address;
                        printf("Reconnected to %s\n", selected.name.c_str());
                        reconnected = true;
                        break;
                    }
                }

                if (reconnected) break;

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            if (reconnected) {
                continue;  // Try to receive again
            }

            // Failed to reconnect within timeout - exit to let systemd restart
            std::cout << "Failed to reconnect within " << (reconnect_timeout_ms / 1000)
                      << "s, exiting for restart\n";
            g_should_close = true;
            break;
        }

        if (g_should_close || display_should_close(display)) {
            break;
        }

        // Wait before checking again
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    printf("Shutting down...\n");
    log("=== Application shutting down ===");

    // Cleanup
    if (audio) {
        audio_stop(audio);
    }
    NDIlib_destroy();
    destroy_display(display);
    if (audio) {
        audio_close(audio);
    }

    log_close();
    return 0;
}
