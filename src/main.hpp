#ifndef H9138789750
#define H9138789750

#include <deque>
#include <filesystem>
#include <optional>

// windows.h must come first
#include <windows.h>

class Process {
public:
    Process(const Process& other) = delete;
    Process(Process&& other) noexcept
        : start_info(other.start_info), proc_info(other.proc_info) {
        ZeroMemory(&other.start_info, sizeof(other.start_info));
        ZeroMemory(&other.proc_info, sizeof(other.proc_info));
        other.start_info.cb = sizeof(other.start_info);
    }

    ~Process() {
        if (proc_info.hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(proc_info.hProcess);
            proc_info.hProcess = INVALID_HANDLE_VALUE;
        }

        if (proc_info.hThread != INVALID_HANDLE_VALUE) {
            CloseHandle(proc_info.hThread);
            proc_info.hThread = INVALID_HANDLE_VALUE;
        }
    }

    auto operator=(const Process& other) -> Process& = delete;
    auto operator=(Process&& other) noexcept -> Process& {
        start_info = other.start_info;
        proc_info = other.proc_info;
        ZeroMemory(&other.start_info, sizeof(other.start_info));
        ZeroMemory(&other.proc_info, sizeof(proc_info));
        other.start_info.cb = sizeof(other.start_info);
        return *this;
    }

    static auto run(const std::filesystem::path& path)
        -> std::optional<Process>;

private:
    Process() {
        ZeroMemory(&start_info, sizeof(start_info));
        ZeroMemory(&proc_info, sizeof(proc_info));
        start_info.cb = sizeof(start_info);
    }

    auto run_exe(const std::filesystem::path& path) -> bool;

    STARTUPINFO start_info = {};
    PROCESS_INFORMATION proc_info = {};
};

auto is_window_active(const LPCSTR win, const bool req_text = true) -> bool;
auto wait_until_window(
    const LPCSTR win, const int timeout_ms, const bool text = true) -> int;

#endif // H9138789750
