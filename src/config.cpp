#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include "config.h"

// Minimal INI-style parser: key=value, one per line, # for comments
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::optional<Config> load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;

    Config cfg;
    cfg.config_path = path;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "source_name") cfg.source_name = val;
        else if (key == "fullscreen") cfg.fullscreen = (val == "true" || val == "1");
        else if (key == "display_index") cfg.display_index = std::stoi(val);
    }

    return cfg;
}

bool save_config(const Config& cfg, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "# ndi-monitor configuration\n";
    file << "source_name=" << cfg.source_name << "\n";
    file << "fullscreen=" << (cfg.fullscreen ? "true" : "false") << "\n";
    file << "display_index=" << cfg.display_index << "\n";

    return file.good();
}
