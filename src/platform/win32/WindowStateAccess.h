#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

inline AppWindowState* appStateForWindow(HWND hwnd) noexcept
{
    return reinterpret_cast<AppWindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

} // namespace ovtr::win32
