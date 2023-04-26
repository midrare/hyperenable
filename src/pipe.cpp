#include <cstdio>
#include <iostream>
#include <optional>
#include <stop_token>

// windows.h must come before other windows headers
#include <windows.h>

#include <fileapi.h>
#include <winuser.h>

#include "pipe.hpp"


constexpr LPCSTR pipe_name = R"(\\.\pipe\e3f5a607-ec52-4841-886d-9502cf837302)";
constexpr char cmd_stop = 0x01;  // arbitrary value


auto Listener::create() -> std::optional<Listener> {
    auto server = std::make_shared<Server>();
    if (!server->init()) {
        return {};
    }

    std::optional<Listener> listener = Listener();
    auto received = listener->received;

    listener->thread = std::make_shared<std::jthread>(
        // NOLINTNEXTLINE(performance-unnecessary-value-param)
        std::jthread([server, received] (const std::stop_token stop) {
        while (!stop.stop_requested()) {
            if (server->poll()) {
                received->store(true);
                break;
            }
        }
    }));

    return listener;
}

auto Listener::poll() -> bool {
    return received->load();
}


auto Server::init() -> bool {
    if (pipe != INVALID_HANDLE_VALUE) {
        return true;
    }

    HANDLE existing = CreateFileA(pipe_name,
        GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, 0, NULL);
    if (existing != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
        existing = INVALID_HANDLE_VALUE;
        return false;
    }

    pipe = CreateNamedPipeA(pipe_name,
       PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
       PIPE_TYPE_BYTE | PIPE_READMODE_BYTE
       | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
       1, bufsize, bufsize,
       NMPWAIT_USE_DEFAULT_WAIT, NULL);

    return pipe != INVALID_HANDLE_VALUE;
};


void Server::deinit() {
    if (pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }
}

auto Server::poll() -> bool {
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    return read_into_buffer();
}

auto Server::read_into_buffer() -> bool {
    buffer[0] = '\0';
    buffer[bufsize] = '\0';
    DWORD bytes_read = 0;
    BOOL status = FALSE;

    if (ConnectNamedPipe(pipe, NULL) != FALSE) {
        status = ReadFile(pipe, buffer.data(),
                   buffer.size() - 1, &bytes_read, NULL);
    }

    DisconnectNamedPipe(pipe);
    return status != FALSE && bytes_read > 0;
}

auto Client::init() -> bool {
    if (pipe != INVALID_HANDLE_VALUE) {
        return true;
    }

    pipe = CreateFileA(pipe_name,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        0, NULL);

    return pipe != INVALID_HANDLE_VALUE;
}

void Client::deinit() {
    if (pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }
}

auto Client::notify() -> bool {
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytes_written = 0;
    std::array<char, bufsize + 1> buf = {};
    buf[bufsize] = '\0';

    buf[0] = cmd_stop;
    buf[1] = '\0';

    WriteFile(pipe, buf.data(), std::strlen(buf.data()), &bytes_written, NULL);

    return bytes_written > 0;
}
