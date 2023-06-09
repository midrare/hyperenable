#ifndef H4685028741
#define H4685028741

#include <filesystem>

#include <windows.h> // must precede other windows headers

#include "yaml.hpp"

struct Config {
    std::vector<std::pair<int, int>> keyboard_shortcuts;
};

auto parse_keybind(const std::string& s) -> std::optional<std::pair<int, int>>;
auto read_config(const std::filesystem::path& path) -> Config;

#endif // H4685028741
