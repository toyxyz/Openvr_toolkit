#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool handleProfilePanelClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleProfilePanelDoubleClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
void saveCurrentProfile(HWND hwnd, AppWindowState& state);
void loadProfileFromDialog(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
