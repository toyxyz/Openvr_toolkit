#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

void handleViewportLeftButtonDown(HWND hwnd, LPARAM lparam);
void handleViewportLeftButtonUp(HWND hwnd);
void handleViewportMiddleButtonDown(HWND hwnd, LPARAM lparam);
void handleViewportMiddleButtonUp(HWND hwnd);
void handleViewportMouseMove(HWND hwnd, LPARAM lparam);
void handleViewportMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam);

} // namespace ovtr::win32
