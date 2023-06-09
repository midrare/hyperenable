#include <algorithm>
#include <deque>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

// windows.h must come first
#include <windows.h>

#include <combaseapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <shellapi.h>
#include <winuser.h>

#include "argparse.hpp"
#include "config.hpp"
#include "keys.hpp"
#include "main.hpp"
#include "pipe.hpp"
#include "squatter.hpp"
#include "wincon.hpp"
#include "yaml.hpp"

const std::string program_name = "hyperenable";
const std::string program_version = "0.1.3";

const LPCSTR window_taskbar = "Shell_TrayWnd";

// class that designates the desktop. this is the borderless window
// that shows your recycling bin and desktop icons
const LPCSTR window_desktop = "Progman";

const std::string action_start = "start";
const std::string action_stop = "stop";

constexpr int timeout_ms = 15000;
constexpr int bufsize = 1024;
constexpr int interval_ms = 100;
constexpr int delay_ms = 0;

constexpr int exit_ok = 0;
constexpr int exit_timeout = 1;
constexpr int exit_explorer_exists = 2;
constexpr int exit_file_not_found = 3;
constexpr int exit_connect_fail = 4;
constexpr int exit_argparse = 5;

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

/* wtos() is from https://stackoverflow.com/a/67134110
 * https://stackoverflow.com/users/870239/liang-august-yuning
 * CC BY-SA 4.0
 */
std::wstring wtos(const std::string& value) {
    const size_t cSize = value.size() + 1;

    std::wstring wc;
    wc.resize(cSize);

    size_t cSize1;
    mbstowcs_s(&cSize1, (wchar_t*)&wc[0], cSize, value.c_str(), cSize);

    wc.pop_back();

    return wc;
}

auto main(int argc, char* argv[]) -> int {
    argparse::ArgumentParser args_start(action_start);
    args_start.add_description("Block keybind registration attempts");
    args_start.add_argument("-t", "--timeout")
        .help("max milliseconds to wait for explorer.exe to load. "
              "negative for infinite")
        .default_value(timeout_ms)
        .scan<'i', int>()
        .metavar("INT");
    args_start.add_argument("-c", "--config")
        .help("read settings from file")
        .metavar("PATH");
    args_start.add_argument("-r", "--run")
        .help("run a program afterwards. can "
              "be used to run a hotkey setup app")
        .metavar("PATH");
    args_start.add_epilog("Keybind registration attempts will be "
                          "blocked until terminated, timeout elapses, "
                          "or stop signal is received, whichever happens "
                          "first.");

    argparse::ArgumentParser args_stop(action_stop);
    args_stop.add_description("Unblock keybind registration attempts");
    args_stop.add_epilog(
        "A signal will be sent to a running "
        "instance of " +
        program_name +
        " to stop "
        "blocking keybinds.");

    argparse::ArgumentParser args(program_name, program_version);
    args.add_subparser(args_start);
    args.add_subparser(args_stop);

    try {
        args.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return exit_argparse;
    }

    if (args.is_subcommand_used(args_start)) {
        if (is_window_active(window_desktop)) {
            std::cerr << "Can only block before explorer.exe "
                         "is active."
                      << std::endl;
            return exit_explorer_exists;
        }

        auto server = Listener::create();
        if (!server) {
            std::cout << "Another instance is already blocking. "
                      << "No need to block again." << std::endl;
            return exit_ok;
        }

        Config config;
        if (args_start.present("--config")) {
            std::filesystem::path path = args_start.get("--config");
            if (!std::filesystem::exists(path)) {
                std::cerr << "File not found at \"" << path << "\""
                          << std::endl;
                return exit_file_not_found;
            }
            config = read_config(path);
        }

        if (config.keyboard_shortcuts.empty()) {
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+a").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+b").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+c").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+d").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+e").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+f").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+g").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+h").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+i").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+j").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+k").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+l").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+m").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+n").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+o").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+p").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+q").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+r").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+s").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+t").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+u").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+v").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+w").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+x").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+y").value());
            config.keyboard_shortcuts.push_back(
                parse_keybind("ctrl+alt+shift+win+z").value());
        }

        std::cout << "Blocking keybinds." << std::endl;
        auto blocker = Squatter::block(config.keyboard_shortcuts);

        auto timeout_ms = args_start.get<int>("--timeout");

        std::cout << "Waiting for explorer.exe" << std::endl;
        auto waited_ms = wait_until_window(window_desktop, timeout_ms);
        if (waited_ms < 0) {
            std::cerr << "Timed out while waiting for explorer.exe."
                      << std::endl;
            return exit_timeout;
        }

        std::cout << "Waiting until release signal or timeout." << std::endl;
        while (waited_ms < timeout_ms && !server->poll()) {
            Sleep(max(0, min(timeout_ms - waited_ms, interval_ms)));
            waited_ms += interval_ms;
        }

        std::cout << "Unblocking keybinds." << std::endl;
        blocker.unblock();

        if (args_start.present("--run")) {
            std::filesystem::path path = args_start.get("--run");
            if (!std::filesystem::exists(path)) {
                std::cerr << "File not found at \"" << path << "\""
                          << std::endl;
                return exit_file_not_found;
            }

            std::cout << "Running program at " << path << std::endl;

            // always init COM before using ShellExecute()
            // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea#remarks
            CoInitializeEx(
                NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            auto status = ShellExecuteW(
                NULL, wtos("Open").c_str(), path.c_str(), NULL, NULL, SW_HIDE);
        }

        return exit_ok;
    } else if (args.is_subcommand_used(args_stop)) {
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
    } else {
        std::cerr << "Unrecognized subcommand." << std::endl;
        return exit_argparse;
    }
}

_Use_decl_annotations_ auto WINAPI WinMain(
    HINSTANCE /*unused*/, HINSTANCE /*unused*/, PSTR /*unused*/, INT /*unused*/)
    -> INT {
    WinConsole console = WinConsole::attach();

    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/argc-argv-wargv
    return main(__argc, __argv);
}
