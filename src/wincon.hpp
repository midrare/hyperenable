#ifndef H5412315995
#define H5412315995

#include <cstdio>

// windows.h must come before other windows headers
#include <windows.h>

class WinConsole {
  public:
    WinConsole(WinConsole &&old) noexcept
        : attached(old.attached), stdin_(old.stdin_), stdout_(old.stdout_),
          stderr_(old.stderr_) {
        old.stdin_ = nullptr;
        old.stdout_ = nullptr;
        old.stderr_ = nullptr;
        old.attached = false;
    }

    WinConsole(const WinConsole &) = delete;
    ~WinConsole() { this->detach_console(); }

    auto operator=(const WinConsole &) = delete;
    auto operator=(WinConsole &) -> WinConsole & = delete;
    auto operator=(WinConsole &&) -> WinConsole & = delete;

    // static "constructor" to make it more obvious this is RAII
    [[maybe_unused]] static auto attach() -> WinConsole { return {}; }

  private:
    bool attached = false;

    FILE* stdin_ = nullptr;
    FILE* stdout_ = nullptr;
    FILE* stderr_ = nullptr;

    [[nodiscard]] [[maybe_unused]] WinConsole() { this->attach_console(); }

    void attach_console();
    void detach_console();
};


#endif // H5412315995
