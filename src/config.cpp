#include <algorithm>
#include <optional>
#include <ranges>
#include <regex>

#include "config.hpp"
#include "keys.hpp"
#include "yaml.hpp"

/* resplit() by https://stackoverflow.com/users/248823/marcin
 * https://stackoverflow.com/a/28142357
 * used under CC BY-SA 4.0 */
auto resplit(const std::string& s, const std::regex& sep)
    -> std::vector<std::string> {
    std::sregex_token_iterator iter(s.begin(), s.end(), sep, -1);
    std::sregex_token_iterator end;
    return {iter, end};
}

void to_lower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
}

auto find(const std::multimap<int, std::string>& m, std::string s)
    -> std::optional<int> {
    to_lower(s);
    for (auto& [vkey, keystr] : m) {
        if (s == keystr) {
            return vkey;
        }
    }

    return {};
}

auto parse_keybind(const std::string& s) -> std::optional<std::pair<int, int>> {
    if (s.empty()) {
        return {};
    }

    int modifiers = 0;
    int keycode = 0;

    auto segs = resplit(s, std::regex("\\s*\\+\\s*"));
    for (auto& seg : segs) {
        if (seg.empty()) {
            continue;
        }

        auto modifier = find(modifier_keycode_to_str, seg);
        if (modifier.has_value()) {
            modifiers |= modifier.value();
            continue;
        }

        auto key = find(keycode_to_str, seg);
        if (key.has_value()) {
            keycode = key.value();
            continue;
        }
    }

    if (modifiers <= 0 or keycode <= 0) {
        return {};
    }

    return std::make_pair(modifiers, keycode);
}

auto read_config(const std::filesystem::path& path) -> Config {
    Yaml::Node root;
    Yaml::Parse(root, path.string());

    Config conf;

    if (root.IsMap() && root["keyboard_shortcuts"].IsSequence()) {
        for (auto i = 0; i < root["keyboard_shortcuts"].Size(); i++) {
            auto& e = root["keyboard_shortcuts"][i];
            auto s = e.As<std::string>();
            auto keybind = parse_keybind(s);
            if (keybind.has_value()) {
                conf.keyboard_shortcuts.emplace_back(keybind.value());
            }
        }
    }

    return std::move(conf);
}
