#pragma once

#include <string>
#include <vector>

struct NdiSourceInfo {
    std::string name;
    std::string url_address;
};

// Discover NDI sources on the network. Blocks until at least one is found or timeout_ms elapses.
std::vector<NdiSourceInfo> discover_sources(int timeout_ms = 10000);

// Print discovered sources to stdout, return index of selected source (-1 if none).
int select_source_cli(const std::vector<NdiSourceInfo>& sources, const std::string& preferred = "");
