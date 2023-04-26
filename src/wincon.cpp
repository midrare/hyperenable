#include <cassert>
#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "wincon.hpp"

void WinConsole::attach_console() {
    if (this->attached) {
        throw std::runtime_error("console is already attached");
    }

    // https://stackoverflow.com/a/26087606
    this->attached =
        static_cast<bool>(AttachConsole(ATTACH_PARENT_PROCESS));
    if (this->attached) {
        assert(this->stdin_ == nullptr);
        assert(this->stdout_ == nullptr);
        assert(this->stderr_ == nullptr);

        _wfreopen_s(&this->stdin_, L"CONIN$", L"r", stdin);
        _wfreopen_s(&this->stdout_, L"CONOUT$", L"w", stdout);
        _wfreopen_s(&this->stderr_, L"CONOUT$", L"w", stderr);
    }
}

void WinConsole::detach_console() {
    if (this->stdin_ != nullptr) {
        fflush(this->stdin_);
        fclose(this->stdin_);
        this->stdin_ = nullptr;
    }

    if (this->stdout_ != nullptr) {
        fflush(this->stdout_);
        fclose(this->stdout_);
        this->stdout_ = nullptr;
    }

    if (this->stderr_ != nullptr) {
        fflush(this->stderr_);
        fclose(this->stderr_);
        this->stderr_ = nullptr;
    }

    if (static_cast<bool>(this->attached)) {
        FreeConsole();
        this->attached = false;
    }
}
