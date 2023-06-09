#include <string>
#include <vector>

// windows.h must be before other windows headers
#include <windows.h>

#include "squatter.hpp"

Squatter::Squatter(const std::vector<std::pair<int, int>>& keys) {
    for (auto& [mods, key] : keys) {
        block(mods, key);
    }
}

Squatter::~Squatter() {
    unblock();
}

void Squatter::unblock() {
    while (!hotkeys.empty()) {
        auto hotkey_id = hotkeys.front();
        hotkeys.pop_front();
        UnregisterHotKey(NULL, hotkey_id);
    }
}

void Squatter::block(const int modifiers, const int key) {
    int hotkey_id = min((std::numeric_limits<int>::max)(), hotkeys.size());
    hotkeys.emplace_back(hotkey_id);
    RegisterHotKey(NULL, hotkey_id, modifiers | MOD_NOREPEAT, key);
}
