#include <filesystem>
#include <iostream>
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
#include "main.hpp"
#include "squatter.hpp"
#include "wincon.hpp"

const std::string program_name = "hyperenable";
const std::string program_version = "0.2.0";

const LPCSTR window_taskbar = "Shell_TrayWnd";

const LPCSTR mutex_name = "Local\\hyperenable_SingleInstance";
const LPCSTR event_name = "Local\\hyperenable_QuitEvent";

// class that designates the desktop. this is the borderless window
// that shows your recycling bin and desktop icons
const LPCSTR window_desktop = "Progman";

constexpr int bufsize = 1024;
constexpr int interval_ms = 100;
constexpr int post_delay_ms = 1000;

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

/* wtos() is from https://stackoverflow.com/a/67134110
 * https://stackoverflow.com/users/870239/liang-august-yuning
 * CC BY-SA 4.0
 */
auto wtos(const std::string& value) -> std::wstring {
    const size_t cSize = value.size() + 1;

    std::wstring wc;
    wc.resize(cSize);

    size_t cSize1 = 0;
    mbstowcs_s(
        &cSize1, static_cast<wchar_t*>(&wc[0]), cSize, value.c_str(), cSize);

    wc.pop_back();

    return wc;
}

auto main(int argc, char* argv[]) -> int {
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
    args.add_argument("--kill")
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

    try {
        args.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << '\n';
        return exit_argparse;
    }

    if (args.get<bool>("--kill")) {
        // check a kill signal
        // this is necessary because Windows provides no easy way to kill a
        // process without flashing a console window
        // we start in a hidden window by default so we can kill our own
        // process safely
        HANDLE event = OpenEventA(EVENT_MODIFY_STATE, FALSE, event_name);
        if (event) {
            SetEvent(event);
            CloseHandle(event);
            event = NULL;
        }

        return exit_ok;
    }

    if (is_window_active(window_desktop)) {
        std::cerr << "Can only block before explorer.exe "
                     "is active.\n";
        return exit_explorer_exists;
    }

    HANDLE mutex = CreateMutexA(nullptr, TRUE, mutex_name);
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Another instance is already blocking. "
                  << "No need to block again.\n";
        if (mutex) {
            CloseHandle(mutex);
            mutex = NULL;
        }
        return exit_ok;
    }

    Config config;
    if (args.present("--config")) {
        std::filesystem::path path = args.get("--config");
        if (!std::filesystem::exists(path)) {
            std::cerr << "File not found at \"" << path << "\"\n";
            return exit_file_not_found;
        }
        config = read_config(path);
    }

    if (config.keyboard_shortcuts.empty()) {
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+w").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+t").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+y").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+o").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+p").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+d").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+l").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+x").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+alt+shift+win+n").value());
        config.keyboard_shortcuts.push_back(
            parse_keybind("ctrl+win+l").value());
    }

    HANDLE quit_event = CreateEventA(NULL, TRUE, FALSE, event_name);
    ResetEvent(quit_event);
    if (!quit_event) {
        std::cerr << "Failed to create quit event.\n";
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        mutex = NULL;
        return exit_connect_fail;
    }

    std::cout << "Acquiring keybinds.\n";
    auto blocker = Squatter::block(config.keyboard_shortcuts);

    auto timeout_ms = args.get<int>("--timeout") * 1000;

    std::cout << "Waiting for explorer.exe until release signal or timeout.\n";
    int64_t waited_ms = 0;
    while (waited_ms < timeout_ms && !is_window_active(window_desktop)) {
        MSG msg;
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY) {
                std::cout << "Hotkey intercepted: 0x" << std::hex << msg.wParam
                          << std::dec << '\n';
            }
        }

        if (WaitForSingleObject(quit_event, 0) == WAIT_OBJECT_0) {
            std::cout << "Quit event received.\n";
            break;
        }

        Sleep(max(0, interval_ms));
        waited_ms += max(0, interval_ms);
    }

    std::cout << "Waited " << waited_ms << "ms / " << timeout_ms << "ms.\n";
    std::cout << "Explorer state: " << is_window_active(window_desktop) << "\n";

    // explorer.exe started but might not have registered hotkeys yet
    // safety wait so its attempt can fail while we still hold those
    // hotkeys
    if (post_delay_ms > 0) {
        std::cout << "Delaying " << post_delay_ms << "ms.\n";
        Sleep(max(0, post_delay_ms));
    }

    std::cout << "Releasing keybinds.\n";
    blocker.unblock();

    std::cout << "Closing quit event.\n";
    ResetEvent(quit_event);
    CloseHandle(quit_event);
    quit_event = NULL;
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    mutex = NULL;

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
