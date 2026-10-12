#ifndef H9138789750
#define H9138789750

// windows.h must come first
#include <string>
#include <windows.h>

auto is_window_active(LPCSTR win, bool req_text = true) -> bool;
auto wtos(const std::string& value) -> std::wstring;

#endif // H9138789750
