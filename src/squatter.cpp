#include <string>
#include <vector>

// windows.h must be before other windows headers
#include <windows.h>

#include "squatter.hpp"

constexpr UINT vk_digit_begin = 0x30;
constexpr UINT vk_digit_end = 0x3a;
constexpr UINT vk_alpha_begin = 0x41;
constexpr UINT vk_alpha_end = 0x5b;

void Squatter::init() {
    block(VK_SPACE);
    block(VK_LWIN);
    block(VK_RWIN);

    for (auto key = vk_digit_begin; key < vk_digit_end; key++) {
        block(key);
    }

    for (auto key = vk_alpha_begin; key < vk_alpha_end; key++) {
        block(key);
    }
}

void Squatter::deinit() {
    while (!hotkeys.empty()) {
        auto key = hotkeys.front();
        hotkeys.pop_front();
        UnregisterHotKey(NULL, key);
    }
}

void Squatter::block(const UINT key) {
    int hotkey_id = min((std::numeric_limits<int>::max)(), hotkeys.size());
    hotkeys.emplace_back(hotkey_id);
    RegisterHotKey(
        NULL, hotkey_id,
        MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_NOREPEAT, key);
}

