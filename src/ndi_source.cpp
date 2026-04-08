#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <Processing.NDI.Lib.h>
#include "ndi_source.h"

std::vector<NdiSourceInfo> discover_sources(int timeout_ms) {
    std::vector<NdiSourceInfo> results;

    if (!NDIlib_initialize()) {
        std::cerr << "Failed to initialize NDI library\n";
        return results;
    }

    NDIlib_find_create_t find_desc = {0};
    NDIlib_find_instance_t finder = NDIlib_find_create_v2(&find_desc);
    if (!finder) {
        std::cerr << "Failed to create NDI finder\n";
        return results;
    }

    // Wait for sources to appear
    int waited = 0;
    while (waited < timeout_ms) {
        NDIlib_find_wait_for_sources(finder, 100);
        waited += 100;

        uint32_t count = 0;
        const NDIlib_source_t* sources = NDIlib_find_get_current_sources(finder, &count);
        if (count > 0) {
            for (uint32_t i = 0; i < count; i++) {
                NdiSourceInfo info;
                info.name = sources[i].p_ndi_name ? sources[i].p_ndi_name : "";
                info.url_address = sources[i].p_url_address ? sources[i].p_url_address : "";
                results.push_back(info);
            }
            break;
        }
    }

    NDIlib_find_destroy(finder);
    return results;
}

int select_source_cli(const std::vector<NdiSourceInfo>& sources, const std::string& preferred) {
    if (sources.empty()) return -1;

    // If a preferred source name is given, try to match it
    if (!preferred.empty()) {
        for (size_t i = 0; i < sources.size(); i++) {
            if (sources[i].name == preferred) {
                printf("Using configured source: %s\n", sources[i].name.c_str());
                return static_cast<int>(i);
            }
        }
        printf("Preferred source '%s' not found, showing list\n", preferred.c_str());
    }

    // List available sources
    printf("\nAvailable NDI sources:\n");
    for (size_t i = 0; i < sources.size(); i++) {
        printf("  [%zu] %s\n", i, sources[i].name.c_str());
    }

    if (sources.size() == 1) {
        printf("Auto-selecting only available source\n");
        return 0;
    }

    printf("\nSelect source [0-%zu]: ", sources.size() - 1);
    int choice = -1;
    if (scanf("%d", &choice) == 1 && choice >= 0 && choice < static_cast<int>(sources.size())) {
        return choice;
    }

    printf("Invalid selection, defaulting to first source\n");
    return 0;
}
