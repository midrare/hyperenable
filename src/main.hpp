#ifndef H9138789750
#define H9138789750

#include <deque>
#include <filesystem>
#include <optional>

// windows.h must come first
#include <windows.h>

auto is_window_active(const LPCSTR win, const bool req_text = true) -> bool;
auto wait_until_window(
    const LPCSTR win, const int timeout_ms, const bool text = true) -> int;
std::wstring wtos(const std::string& value);

#endif // H9138789750
