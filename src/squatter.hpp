#ifndef H3998501979
#define H3998501979

#include <deque>

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

    ~Squatter() { unblock(); }

    auto operator=(const Squatter& other) -> Squatter& = delete;
    auto operator=(Squatter&& other) noexcept -> Squatter& {
        for (auto e : other.hotkeys) {
            hotkeys.emplace_back(e);
        }
        other.hotkeys.clear();
        return *this;
    }

    static auto block() -> Squatter { return std::move(Squatter()); };

    void block(const UINT key);
    auto unblock() -> void { deinit(); };

private:
    Squatter() { init(); }

    void init();
    void deinit();

    std::deque<int> hotkeys;
};

#endif // H3998501979
