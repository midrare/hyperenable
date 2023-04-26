#ifndef H6193606644
#define H6193606644

#include <array>
#include <optional>
#include <thread>

// windows.h must come before other windows headers
#include <windows.h>

#include <fileapi.h>
#include <winuser.h>

class Listener {
public:
    static auto create() -> std::optional<Listener>;

    Listener(const Listener& other) = delete;
    Listener(Listener&& other) noexcept
        : thread(std::move(other.thread)), received(std::move(other.received)) {
        other.thread = nullptr;
        other.received = nullptr;
    };

    ~Listener() {
        if (thread) {
            thread->request_stop();
            thread = nullptr;
        }
    }

    auto operator=(const Listener& other) -> Listener& = delete;
    auto operator=(Listener&& other) noexcept -> Listener& {
        thread = other.thread;
        received = other.received;
        other.thread = nullptr;
        other.received = nullptr;
        return *this;
    };

    auto poll() -> bool;

private:
    Listener() = default;

    std::shared_ptr<std::jthread> thread = nullptr;
    std::shared_ptr<std::atomic_bool> received =
        std::make_shared<std::atomic_bool>(false);
};

class Server {
public:
    Server() = default;

    Server(const Server& other) = delete;
    Server(Server&& other) noexcept : pipe(other.pipe) {
        other.pipe = INVALID_HANDLE_VALUE;
    };

    auto operator=(const Server& other) -> Server& = delete;
    auto operator=(Server&& other) noexcept -> Server& {
        pipe = other.pipe;
        other.pipe = INVALID_HANDLE_VALUE;
        return *this;
    };

    ~Server() { deinit(); }

    auto init() -> bool;
    void deinit();

    auto poll() -> bool;

private:
    static const DWORD bufsize = 1024;
    static const int interval_ms = 100;

    auto read_into_buffer() -> bool;

    HANDLE pipe = INVALID_HANDLE_VALUE;
    std::array<char, bufsize + 1> buffer = {};
};

class Client {
public:
    Client() = default;

    Client(const Client& other) = delete;
    Client(Client&& other) noexcept : pipe(other.pipe) {
        other.pipe = INVALID_HANDLE_VALUE;
    }

    auto operator=(const Client& other) -> Client& = delete;
    auto operator=(Client&& other) noexcept -> Client& {
        pipe = other.pipe;
        other.pipe = INVALID_HANDLE_VALUE;
        return *this;
    }

    ~Client() { deinit(); }

    auto init() -> bool;
    void deinit();

    auto notify() -> bool;

private:
    static const DWORD bufsize = 1024;

    HANDLE pipe = INVALID_HANDLE_VALUE;
};

#endif // H6193606644
