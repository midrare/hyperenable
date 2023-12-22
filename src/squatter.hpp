#ifndef H3998501979
#define H3998501979

#include <deque>
#include <iostream>
#include <string>
#include <vector>

// windows.h must come before other windows headers
#include <windows.h>

class Squatter {
public:
    Squatter(const Squatter& other) = delete;
    Squatter(Squatter&& other) noexcept {
        hotkeys = std::move(other.hotkeys);
    }

    ~Squatter();

    auto operator=(const Squatter& other) -> Squatter& = delete;
    auto operator=(Squatter&& other) noexcept -> Squatter& {
        hotkeys = std::move(other.hotkeys);
        return *this;
    }

    static auto block(const std::vector<std::pair<int, int>>& keys)
        -> Squatter {
        Squatter squatter(keys);
        return std::move(squatter);
    };

    auto unblock() -> void;

private:
    Squatter(const std::vector<std::pair<int, int>>& keys);

    void block(const int modifiers, const int key);

    std::deque<int> hotkeys;
};

#endif // H3998501979
