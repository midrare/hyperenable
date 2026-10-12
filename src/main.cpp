#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

// windows.h must come first
#include <windows.h>

#include <handleapi.h>
#include <synchapi.h>
#include <winuser.h>

#include "argparse.hpp"
#include "config.hpp"
#include "main.hpp"
#include "squatter.hpp"
#include "wincon.hpp"

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------

const std::string program_name = "hyperenable";
const std::string program_version = "0.2.0";

// process exit codes
constexpr int exit_ok = 0;
constexpr int exit_timeout = 1;
constexpr int exit_explorer_exists = 2;
constexpr int exit_file_not_found = 3;
constexpr int exit_connect_fail = 4;
constexpr int exit_argparse = 5;

// explorer.exe window classes. represents the borderless surface
// the desktop icons appear on; it only exists once explorer.exe is running
const LPCSTR window_desktop = "Progman";

// named kernel objects shared between invocations
// mutex marks that an instance is already blocking
// event tells the blocking instance to stop
const LPCSTR mutex_name = "Local\\hyperenable_SingleInstance";
const LPCSTR event_name = "Local\\hyperenable_QuitEvent";

constexpr int bufsize = 1024;
constexpr int interval_ms = 100;
constexpr int post_delay_ms = 1000;

// ---------------------------------------------------------------------------
// win32 helpers
// ---------------------------------------------------------------------------

auto is_window_active(const LPCSTR win, const bool req_text) -> bool {
    auto* explorer = FindWindow(win, NULL);
    if (explorer == NULL) {
        return false;
    }

    if (req_text) {
        std::array<char, bufsize> buf{};
        std::size_t read_len = GetWindowText(explorer, buf.data(), buf.size());
        if (read_len <= 0) {
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// command line
// ---------------------------------------------------------------------------

auto build_arg_parser() -> argparse::ArgumentParser {
    argparse::ArgumentParser args(
        program_name, program_version, argparse::default_arguments::none);

    args.add_description(
        "Block default keybind registration attempts from Windows\n\n"
        "Windows reserves certain hotkeys for its own use. These include "
        "the Office key (win+shift+ctrl+alt) among many others. To unbind "
        "these, " +
        program_name +
        " starts before explorer.exe and "
        "registers them before Windows can, like a domain squatter. "
        "After " +
        program_name +
        " exits, these keybinds can then rebound "
        "for your own use.");
    args.add_argument("-t", "--timeout")
        .help(
            "seconds to wait to for explorer.exe to load. negative waits "
            "forever")
        .default_value(30)
        .scan<'i', int>()
        .metavar("INT");
    args.add_argument("-c", "--config")
        .help(
            "read given config file that controls which keybinds are affected. "
            "this allows you to control which of Windows default keybinds are "
            "allowed and which ones " +
            program_name + " unbinds for you.")
        .metavar("PATH");
    args.add_argument("--request-exit")
        .help(
            "unblock keybind registration attempts. a signal will be sent "
            "to a running instance of " +
            program_name +
            " to stop blocking keybinds. you normally do "
            "not need to call this yourself")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);
    args.add_epilog(
        "Keybind registration attempts will be blocked until " + program_name +
        " is killed, timeout elapses, or stop signal is received, "
        "whichever happens first.");

    return args;
}

// ---------------------------------------------------------------------------
// other instances
// ---------------------------------------------------------------------------

auto signal_running_instance() -> void {
    HANDLE event = OpenEventA(EVENT_MODIFY_STATE, FALSE, event_name);
    if (event != NULL) {
        SetEvent(event);
        CloseHandle(event);
        event = NULL;
    }
}

auto acquire_instance_mutex() -> HANDLE {
    HANDLE mutex = CreateMutexA(NULL, TRUE, mutex_name);
    if (mutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Another instance is already blocking. "
                  << "No need to block again.\n";
        if (mutex != NULL) {
            CloseHandle(mutex);
        }
        return NULL;
    }

    return mutex;
}

auto create_quit_event() -> HANDLE {
    HANDLE event = CreateEventA(NULL, TRUE, FALSE, event_name);
    if (event == NULL) {
        std::cerr << "Failed to create quit event.\n";
    }

    return event;
}

// ---------------------------------------------------------------------------
// keybinds
// ---------------------------------------------------------------------------

auto default_keybinds() -> std::vector<std::pair<int, int>> {
    return {
        parse_keybind("ctrl+alt+shift+win+w").value(),
        parse_keybind("ctrl+alt+shift+win+t").value(),
        parse_keybind("ctrl+alt+shift+win+y").value(),
        parse_keybind("ctrl+alt+shift+win+o").value(),
        parse_keybind("ctrl+alt+shift+win+p").value(),
        parse_keybind("ctrl+alt+shift+win+d").value(),
        parse_keybind("ctrl+alt+shift+win+l").value(),
        parse_keybind("ctrl+alt+shift+win+x").value(),
        parse_keybind("ctrl+alt+shift+win+n").value(),
        parse_keybind("ctrl+win+l").value(),
    };
}

auto load_config(const argparse::ArgumentParser& args)
    -> std::optional<Config> {
    Config config;

    if (args.present("--config")) {
        std::filesystem::path path = args.get("--config");
        if (!std::filesystem::exists(path)) {
            std::cerr << "File not found at \"" << path << "\"\n";
            return std::nullopt;
        }
        config = read_config(path);
    }

    if (config.keyboard_shortcuts.empty()) {
        config.keyboard_shortcuts = default_keybinds();
    }

    return config;
}

// ---------------------------------------------------------------------------
// blocking run
// ---------------------------------------------------------------------------

auto print_intercepted_hotkey() -> void {
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_HOTKEY) {
        std::cout << "Hotkey intercepted: 0x" << std::hex << msg.wParam
                  << std::dec << '\n';
    }
}

auto wait_for_explorer(HANDLE quit_event, int64_t timeout_ms) -> void {
    std::cout << "Waiting for explorer.exe until release signal or timeout.\n";

    int64_t waited_ms = 0;
    while (waited_ms < timeout_ms && !is_window_active(window_desktop)) {
        print_intercepted_hotkey();

        if (WaitForSingleObject(quit_event, 0) == WAIT_OBJECT_0) {
            std::cout << "Quit event received.\n";
            break;
        }

        Sleep(max(0, interval_ms));
        waited_ms += max(0, interval_ms);
    }

    std::cout << "Waited " << waited_ms << "ms / " << timeout_ms << "ms.\n";
    std::cout << "Explorer state: " << is_window_active(window_desktop) << "\n";
}

// ---------------------------------------------------------------------------
// entry points
// ---------------------------------------------------------------------------

auto main(int argc, char* argv[]) -> int {
    auto args = build_arg_parser();

    try {
        args.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << '\n';
        return exit_argparse;
    }

    if (args.get<bool>("--request-exit")) {
        signal_running_instance();
        return exit_ok;
    }

    // blocking only works before explorer.exe registers its own hotkeys
    if (is_window_active(window_desktop)) {
        std::cerr << "Can only block before explorer.exe is active.\n";
        return exit_explorer_exists;
    }

    HANDLE mutex = acquire_instance_mutex();
    if (mutex == NULL) {
        return exit_ok;
    }

    auto config = load_config(args);
    if (!config) {
        return exit_file_not_found;
    }

    HANDLE quit_event = create_quit_event();
    if (quit_event == NULL) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return exit_connect_fail;
    }

    std::cout << "Acquiring keybinds.\n";
    auto blocker = Squatter::block(config->keyboard_shortcuts);

    const auto timeout_ms = args.get<int>("--timeout") * 1000;
    wait_for_explorer(quit_event, timeout_ms);

    // explorer.exe started but might not have registered hotkeys yet
    // wait so its attempt can fail while we still hold those hotkeys
    if (post_delay_ms > 0) {
        std::cout << "Delaying " << post_delay_ms << "ms.\n";
        Sleep(max(0, post_delay_ms));
    }

    std::cout << "Releasing keybinds.\n";
    blocker.unblock();

    std::cout << "Closing quit event.\n";
    CloseHandle(quit_event);
    ReleaseMutex(mutex);
    CloseHandle(mutex);

    std::cout << "Exiting.\n";
    return exit_ok;
}

_Use_decl_annotations_ auto WINAPI WinMain(
    HINSTANCE /*unused*/, HINSTANCE /*unused*/, PSTR /*unused*/, INT /*unused*/)
    -> INT {
    WinConsole console = WinConsole::attach();

    // https://learn.microsoft.com/en-us/cpp/c-runtime-library/argc-argv-wargv
    return main(__argc, __argv);
}
