#include <gtest/gtest.h>
#include <pwd.h>
#include <unistd.h>
#include "user_detect.h"

TEST(UserDetectTest, DetectCurrentUser) {
    auto user = detect_logged_in_user();
    ASSERT_TRUE(user.has_value());
    EXPECT_FALSE(user->username.empty());
    EXPECT_EQ(user->uid, getuid());
    EXPECT_EQ(user->gid, getgid());
    EXPECT_FALSE(user->home_dir.empty());
    EXPECT_EQ(user->home_dir, getenv("HOME") ? getenv("HOME") : "");
}

TEST(UserDetectTest, UserInfoHasRequiredFields) {
    auto user = detect_logged_in_user();
    ASSERT_TRUE(user.has_value());
    
    EXPECT_FALSE(user->username.empty());
    EXPECT_GT(user->uid, 0);
    EXPECT_GT(user->gid, 0);
    EXPECT_FALSE(user->home_dir.empty());
}

TEST(UserDetectTest, SwitchToUserChangesUid) {
    auto user = detect_logged_in_user();
    ASSERT_TRUE(user.has_value());
    
    uid_t original_uid = getuid();
    gid_t original_gid = getgid();
    
    bool switched = switch_to_user(*user);
    EXPECT_TRUE(switched);
    
    EXPECT_EQ(getuid(), user->uid);
    EXPECT_EQ(getgid(), user->gid);
    
    if (original_uid != user->uid) {
        setuid(original_uid);
        setgid(original_gid);
    }
}