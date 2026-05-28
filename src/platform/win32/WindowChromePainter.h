#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppDebugUiState;
struct AppTopBarState;
struct AppWindowState;

void paintTopBar(HDC drawDc, HFONT font, const AppTopBarState* state, int clientWidth, int clientHeight);
void paintDeviceRailAndSplitter(
    HDC drawDc,
    HFONT font,
    const AppDebugUiState& state,
    int clientWidth,
    int clientHeight,
    int contentBottom
);
void paintDeviceRailAndSplitter(
    HDC drawDc,
    HFONT font,
    AppWindowState& state,
    int clientWidth,
    int clientHeight,
    int contentBottom
);

} // namespace ovtr::win32
