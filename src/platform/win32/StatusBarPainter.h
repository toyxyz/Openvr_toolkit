#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

namespace ovtr::win32 {

struct AppWindowState;

struct StatusBarPaintState {
    std::wstring statusMessage;
    std::wstring statusMetrics;
    bool debugMonitorVisible = false;
};

void paintStatusBar(
    HDC drawDc,
    HFONT font,
    const StatusBarPaintState* state,
    int clientWidth,
    int clientHeight,
    int statusBarTop
);
void paintStatusBar(
    HDC drawDc,
    HFONT font,
    const AppWindowState* state,
    int clientWidth,
    int clientHeight,
    int statusBarTop
);

} // namespace ovtr::win32
