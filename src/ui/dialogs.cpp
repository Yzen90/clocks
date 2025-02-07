#include "dialogs.hpp"

#include <Windows.h>

#include <nowide/convert.hpp>

using nowide::widen;

void error_message(const string& message, const string& title, void*& window_handle) {
  MessageBoxW(static_cast<HWND>(window_handle), widen(message).data(), widen(title).data(), MB_OK | MB_ICONERROR);
}
