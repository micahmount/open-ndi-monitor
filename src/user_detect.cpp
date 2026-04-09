#include "user_detect.h"
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sstream>
#include <string>

std::optional<UserInfo> detect_logged_in_user() {
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    
    std::string username;
    uid_t uid = getuid();
    
    if (xdg_runtime && strlen(xdg_runtime) > 0) {
        std::string runtime_dir(xdg_runtime);
        size_t pos = runtime_dir.rfind('/');
        if (pos != std::string::npos && pos + 1 < runtime_dir.length()) {
            username = runtime_dir.substr(pos + 1);
            
            // Check if it's a numeric UID (e.g., "1000")
            if (username.find_first_not_of("0123456789") == std::string::npos) {
                uid = static_cast<uid_t>(std::stoi(username));
                username.clear();
            }
        }
    }
    
    // Fallback to getpwuid if username not determined from XDG_RUNTIME_DIR
    struct passwd* pw = nullptr;
    if (!username.empty()) {
        pw = getpwnam(username.c_str());
    }
    
    if (!pw) {
        pw = getpwuid(uid);
    }
    
    if (!pw) {
        std::cerr << "Cannot find user for uid: " << uid << "\n";
        return std::nullopt;
    }

    UserInfo info;
    info.username = pw->pw_name ? pw->pw_name : "";
    info.uid = pw->pw_uid;
    info.gid = pw->pw_gid;
    info.home_dir = pw->pw_dir ? pw->pw_dir : "";
    
    if (xdg_runtime && strlen(xdg_runtime) > 0) {
        info.xdg_runtime_dir = xdg_runtime;
    } else {
        info.xdg_runtime_dir = "/run/user/" + std::to_string(info.uid);
    }

    const char* display = getenv("DISPLAY");
    info.display = display ? display : "";

    info.xauthority = info.home_dir + "/.Xauthority";

    return info;
}

bool switch_to_user(const UserInfo& user) {
    if (setgid(user.gid) != 0) {
        std::cerr << "Failed to set gid: " << user.gid << "\n";
        return false;
    }

    if (setuid(user.uid) != 0) {
        std::cerr << "Failed to set uid: " << user.uid << "\n";
        return false;
    }

    return true;
}