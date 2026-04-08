#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "config.h"

namespace {
std::string temp_file() {
    return std::filesystem::temp_directory_path() / "test_config.ini";
}
}

TEST(ConfigTest, LoadNonExistentReturnsNullopt) {
    EXPECT_EQ(load_config("/nonexistent/path.ini"), std::nullopt);
}

TEST(ConfigTest, LoadValidFile) {
    auto path = temp_file();
    std::ofstream(path) << R"(
source_name=Test Source
fullscreen=true
display_index=1
)";
    auto cfg = load_config(path);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->source_name, "Test Source");
    EXPECT_TRUE(cfg->fullscreen);
    EXPECT_EQ(cfg->display_index, 1);
    std::filesystem::remove(path);
}

TEST(ConfigTest, LoadDefaults) {
    auto path = temp_file();
    std::ofstream(path) << "source_name=Test\n";
    auto cfg = load_config(path);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_TRUE(cfg->fullscreen);
    EXPECT_EQ(cfg->display_index, 0);
    std::filesystem::remove(path);
}

TEST(ConfigTest, LoadCommentsIgnored) {
    auto path = temp_file();
    std::ofstream(path) << R"(
# comment
source_name=Valid
# another comment
)";
    auto cfg = load_config(path);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->source_name, "Valid");
    std::filesystem::remove(path);
}

TEST(ConfigTest, LoadInvalidLinesIgnored) {
    auto path = temp_file();
    std::ofstream(path) << "noequals\ninvalid:format\nsource_name=Good\n";
    auto cfg = load_config(path);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->source_name, "Good");
    std::filesystem::remove(path);
}

TEST(ConfigTest, SaveAndLoad) {
    auto path = temp_file();
    Config src{"My Source", false, 2, ""};
    ASSERT_TRUE(save_config(src, path));
    auto loaded = load_config(path);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->source_name, "My Source");
    EXPECT_FALSE(loaded->fullscreen);
    EXPECT_EQ(loaded->display_index, 2);
    std::filesystem::remove(path);
}

TEST(ConfigTest, FullscreenFalseFromZero) {
    auto path = temp_file();
    std::ofstream(path) << "fullscreen=0\n";
    auto cfg = load_config(path);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->fullscreen);
    std::filesystem::remove(path);
}

TEST(ConfigTest, FullscreenFalseFromFalse) {
    auto path = temp_file();
    std::ofstream(path) << "fullscreen=false\n";
    auto cfg = load_config(path);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->fullscreen);
    std::filesystem::remove(path);
}