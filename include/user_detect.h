#pragma once

#include <string>
#include <optional>

struct UserInfo {
    std::string username;
    uid_t uid;
    gid_t gid;
    std::string home_dir;
    std::string xauthority;
    std::string display;
    std::string xdg_runtime_dir;
};

std::optional<UserInfo> detect_logged_in_user();

bool switch_to_user(const UserInfo& user);