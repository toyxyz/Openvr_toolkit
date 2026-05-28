#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool registerOriginDialogClass(HINSTANCE instance);
bool showOriginSettings(HWND parent, AppWindowState& state);

} // namespace ovtr::win32
