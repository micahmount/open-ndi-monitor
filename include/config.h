#pragma once

#include <string>
#include <optional>

struct Config {
    std::string source_name;        // NDI source to connect to (empty = first available)
    bool fullscreen = true;
    int display_index = 0;          // which monitor to use
    std::string audio_device;       // audio device name (empty = default)
    std::string config_path;        // path to config file
};

std::optional<Config> load_config(const std::string& path);
bool save_config(const Config& cfg, const std::string& path);
