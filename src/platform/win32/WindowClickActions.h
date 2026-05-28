#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool handleMainWindowDoubleClickAtPoint(
    HWND hwnd,
    AppWindowState& state,
    int clientWidth,
    int clientHeight,
    POINT point
);

bool handleMainWindowLeftClickAtPoint(
    HWND hwnd,
    AppWindowState& state,
    int clientWidth,
    int clientHeight,
    POINT point
);

} // namespace ovtr::win32
