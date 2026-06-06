#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void applyConfiguredAppIcon(HWND hwnd, AppWindowState& state);
void destroyConfiguredAppIcon(AppWindowState& state) noexcept;

} // namespace ovtr::win32
