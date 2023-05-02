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

#include <fileapi.h>
#include <handleapi.h>
#include <winuser.h>

#include "argparse.hpp"
#include "main.hpp"
#include "pipe.hpp"
#include "squatter.hpp"
#include "wincon.hpp"
#include "yaml.hpp"

const std::string program_name = "hyperenable";
const std::string program_version = "0.1.1";

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


auto main(int argc, char* argv[]) -> int {
    argparse::ArgumentParser args_start(action_start);
    args_start.add_description("Block keybind registration attempts");
    args_start.add_argument("-t", "--timeout")
        .help("max milliseconds to wait for explorer.exe to load. "
              "negative for infinite")
        .default_value(timeout_ms)
        .scan<'i', int>()
        .metavar("INT");
    args_start.add_argument("-r", "--run")
        .help("run a program afterwards. can "
              "be used to run a hotkey setup app")
        .metavar("FILE");
    args_start.add_epilog("Keybind registration attempts will be "
                          "blocked until terminated, timeout elapses, "
                          "or stop signal is received, whichever happens "
                          "first.");

    argparse::ArgumentParser args_stop(action_stop);
    args_stop.add_description("Unblock keybind registration attempts");
    args_stop.add_epilog("A signal will be sent to a running "
                         "instance of " + program_name + " to stop "
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
            std::cout
                << "Another instance is already blocking. "
                << "No need to block again."
                << std::endl;
            return exit_ok;
        }

        std::cout << "Blocking keybinds." << std::endl;
        auto blocker = Squatter::block();
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

        if (!args_start.get("--run").empty()) {
            std::filesystem::path path = args_start.get("--run");
            if (!std::filesystem::exists(path)) {
                std::cerr << "File not found at \""
                    << args_start.get("--run") << "\""
                    << std::endl;
                return exit_file_not_found;
            }

            std::cout << "Running program at " << path << std::endl;
            auto runner = Process::run(path);
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
