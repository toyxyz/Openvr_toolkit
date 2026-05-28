#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void showTopSettingsMenu(HWND hwnd, AppWindowState& state, const RECT& settingRect);
void showTopFileMenu(HWND hwnd, AppWindowState& state, const RECT& fileRect);

} // namespace ovtr::win32
