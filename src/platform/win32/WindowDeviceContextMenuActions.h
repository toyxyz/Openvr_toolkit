#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool executeDeviceContextMenuCommand(HWND hwnd, AppWindowState& state, UINT command);

} // namespace ovtr::win32
