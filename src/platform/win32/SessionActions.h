#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool loadSelectedSessionFolder(HWND hwnd, AppWindowState& state);
bool deleteSelectedSessionFolder(HWND hwnd, AppWindowState& state);
bool closeLoadedSessionFolder(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
