#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

bool handleMainWindowSetCursor(HWND hwnd, LPARAM lparam);
bool handleMainWindowMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam);
bool handleMainWindowMouseMove(HWND hwnd, LPARAM lparam);
bool handleMainWindowLButtonDoubleClick(HWND hwnd, LPARAM lparam);
bool handleMainWindowLButtonDown(HWND hwnd, LPARAM lparam);
bool handleMainWindowRButtonDown(HWND hwnd, LPARAM lparam);
bool handleMainWindowLButtonUp(HWND hwnd);
void handleMainWindowCaptureChanged(HWND hwnd, LPARAM lparam);
bool handleMainWindowKeyDown(HWND hwnd, WPARAM wparam);

} // namespace ovtr::win32
