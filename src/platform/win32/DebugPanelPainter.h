#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void paintDebugMonitorPanel(
    HDC drawDc,
    HFONT titleFont,
    HFONT bodyFont,
    AppWindowState& state,
    int clientWidth,
    int clientHeight,
    int debugMonitorTop,
    int statusBarTop
);

} // namespace ovtr::win32
