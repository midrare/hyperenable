#ifndef H3998501979
#define H3998501979

#include <deque>
#include <string>
#include <vector>

// windows.h must come before other windows headers
#include <windows.h>

class Squatter {
public:
    Squatter(const Squatter& other) = delete;
    Squatter(Squatter&& other) noexcept {
        for (auto e : other.hotkeys) {
            hotkeys.emplace_back(e);
        }
        other.hotkeys.clear();
    }

    ~Squatter();

    auto operator=(const Squatter& other) -> Squatter& = delete;
    auto operator=(Squatter&& other) noexcept -> Squatter& {
        for (auto e : other.hotkeys) {
            hotkeys.emplace_back(e);
        }
        other.hotkeys.clear();
        return *this;
    }

    static auto block(const std::vector<std::pair<UINT, UINT>>& keys)
        -> Squatter {
        Squatter squatter(keys);
        return std::move(squatter);
    };

    auto unblock() -> void;

private:
    Squatter(const std::vector<std::pair<UINT, UINT>>& keys);

    void block(const UINT modifiers, const UINT key);

    std::deque<int> hotkeys;
};

#endif // H3998501979
