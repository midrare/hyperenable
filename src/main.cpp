#include <deque>
#include <filesystem>
#include <limits>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

// windows.h must come first
#include <windows.h>

#include <fileapi.h>
#include <handleapi.h>
#include <winuser.h>

#include "argparse.hpp"
#include "main.hpp"
#include "pipe.hpp"
#include "wincon.hpp"

const std::string program_name = "hyperenable";
const std::string program_version = "0.1.0";

const LPCSTR window_taskbar = "Shell_TrayWnd";

// class that designates the desktop. this is the borderless window
// that shows your recycling bin and desktop icons
const LPCSTR window_desktop = "Progman";

constexpr UINT vk_digit_begin = 0x30;
constexpr UINT vk_digit_end = 0x3a;
constexpr UINT vk_alpha_begin = 0x41;
constexpr UINT vk_alpha_end = 0x5b;

constexpr int timeout_ms = 60000;
constexpr int bufsize = 1024;
constexpr int interval_ms = 100;
constexpr int delay_ms = 0;

constexpr int exit_ok = 0;
constexpr int exit_timeout = 1;
constexpr int exit_explorer_exists = 2;
constexpr int exit_file_not_found = 3;
constexpr int exit_connect_fail = 4;
constexpr int exit_argparse = 5;

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
        MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_NOREPEAT,
        key);
}

auto Process::run(const std::filesystem::path& path) -> std::optional<Process> {
    Process runner;
    if (!runner.run_exe(path)) {
        return {};
    }

    return runner;
}

auto Process::run_exe(const std::filesystem::path& path) -> bool {
    return CreateProcess(
               path.string().c_str(), NULL, NULL, NULL, FALSE,
               NORMAL_PRIORITY_CLASS, NULL, NULL, &start_info,
               &proc_info) == TRUE;
}

auto is_window_active(const LPCSTR win, const bool req_text) -> bool {
    auto* explorer = FindWindow(win, NULL);
    if (explorer == NULL) {
        return false;
    }

    if (req_text) {
        std::size_t read_len = 0;
        std::array<char, bufsize> buf{};
        read_len = GetWindowText(explorer, buf.data(), buf.size());
        if (read_len <= 0) {
            return false;
        }
    }

    return true;
}

auto wait_until_window(const LPCSTR win, const int timeout_ms, const bool text)
    -> int {
    int waited_ms = 0;
    HWND explorer = NULL;

    do {
        explorer = FindWindow(win, NULL);
        if (explorer == NULL) {
            auto wait_ms = min(interval_ms, timeout_ms - waited_ms);
            Sleep(max(0, wait_ms));
            waited_ms += wait_ms;
        }
    } while (explorer == NULL && (timeout_ms < 0 || waited_ms < timeout_ms));

    if (explorer == NULL) {
        return -1;
    }

    if (text) {
        std::size_t read_len = 0;

        do {
            std::array<char, bufsize> buf{};
            read_len = GetWindowText(explorer, buf.data(), buf.size());

            if (read_len <= 0) {
                Sleep(max(0, min(interval_ms, timeout_ms - waited_ms)));
                waited_ms += interval_ms;
            }
        } while (read_len <= 0 && (timeout_ms < 0 || waited_ms < timeout_ms));

        if (read_len <= 0) {
            return -1;
        }
    }

    return min((std::numeric_limits<int>::max)(), waited_ms);
}

auto parse_args(int argc, char* argv[]) -> argparse::ArgumentParser {
    argparse::ArgumentParser program(program_name, program_version);
    program.add_argument("-t", "--timeout")
        .help("max time to wait for explorer.exe to load. in milliseconds. "
              "negative for infinite")
        .default_value(timeout_ms)
        .scan<'i', int>()
        .metavar("INT");
    program.add_argument("--delay")
        .help("wait a while after explorer.exe is active before releasing "
              "keybinds. in milliseconds")
        .default_value(delay_ms)
        .scan<'i', int>()
        .metavar("INT");
    program.add_argument("-r", "--run")
        .help("run a program after this one terminates")
        .metavar("PATH");
    program.add_argument("--release")
        .help("release keybinds")
        .implicit_value(true)
        .default_value(false);
    program.add_epilog("The -r parameter is can be used to run "
                       "a hotkey setup program. This makes it easy to "
                       "time its execution until after this program "
                       "has made sure it's safe.");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::exit(1);
    }

    return program;
}

auto main(int argc, char* argv[]) -> int {
    auto args = parse_args(argc, argv);
    if (args.get<bool>("--release")) {
        auto client = Client();
        if (!client.init()) {
            std::cout << "Not blocking; no need to release." << std::endl;
            return exit_ok;
        }

        if (!client.notify()) {
            std::cerr << "Failed to release keybinds. "
                         "(Pipe communication failed.)"
                      << std::endl;
            return exit_connect_fail;
        }

        return exit_ok;
    }

    if (is_window_active(window_desktop)) {
        std::cerr << "Can only block before explorer.exe "
                     "is active."
                  << std::endl;
        return exit_explorer_exists;
    }

    auto server = Listener::create();
    if (!server) {
        std::cout
            << "Another instance is already blocking. No need to block again."
            << std::endl;
        return exit_ok;
    }


    std::cout << "Blocking keybinds." << std::endl;
    auto blocker = Squatter::block();
    auto timeout_ms = args.get<int>("--timeout");

    std::cout << "Waiting for explorer.exe" << std::endl;
    auto waited_ms = wait_until_window(window_desktop, timeout_ms);
    if (waited_ms < 0) {
        std::cerr << "Timed out while waiting for explorer.exe." << std::endl;
        return exit_timeout;
    }

    std::cout << "Waiting until release signal or timeout." << std::endl;
    while (waited_ms < timeout_ms && !server->poll()) {
        Sleep(max(0, min(timeout_ms - waited_ms, interval_ms)));
        waited_ms += interval_ms;
    }

    auto delay_ms = args.get<int>("--delay");
    if (delay_ms > 0) {
        std::cout << "Delaying." << std::endl;
        Sleep(delay_ms);
    }

    std::cout << "Unblocking keybinds." << std::endl;
    blocker.unblock();

    if (!args.get("run").empty()) {
        std::filesystem::path path = args.get("run");
        if (!std::filesystem::exists(path)) {
            std::cerr << "File not found at \"" << args.get("run") << "\""
                      << std::endl;
            return exit_file_not_found;
        }

        std::cout << "Running program at " << path << std::endl;
        auto runner = Process::run(path);
    }

    return exit_ok;
}

_Use_decl_annotations_ auto WINAPI WinMain(
    HINSTANCE /*unused*/,
    HINSTANCE /*unused*/,
    PSTR /*unused*/,
    INT /*unused*/) -> INT {
    WinConsole console = WinConsole::attach();

    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/argc-argv-wargv
    return main(__argc, __argv);
}

