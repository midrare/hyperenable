#include <vector>

// windows.h must be before other windows headers
#include <windows.h>

#include "squatter.hpp"

Squatter::Squatter(const std::vector<std::pair<int, int>>& keys) {
    for (const auto& [mods, key] : keys) {
        block(mods, key);
    }
}

Squatter::~Squatter() {
    unblock();
}

void Squatter::unblock() {
    while (!hotkeys.empty()) {
        auto [hotkey_id, key, modifiers] = hotkeys.front();
        hotkeys.pop_front();
        // std::cout << "Releasing keybind 0x" << std::hex << key << " 0x"
        //           << modifiers << std::dec << '\n';
        UnregisterHotKey(NULL, hotkey_id);
    }
}

void Squatter::block(const int modifiers, const int key) {
    int hotkey_id = min((std::numeric_limits<int>::max)(), hotkeys.size());
    hotkeys.emplace_back(hotkey_id, key, modifiers | MOD_NOREPEAT);
    // std::cout << "Acquiring keybind 0x" << std::hex << key << " 0x"
    //           << (modifiers | MOD_NOREPEAT) << std::dec << '\n';
    RegisterHotKey(NULL, hotkey_id, modifiers | MOD_NOREPEAT, key);
}
