#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool showViewportColorSettings(HWND parent, AppWindowState& state);
bool showExportLocationSettings(HWND parent, AppWindowState& state);
bool showStreamingSettings(HWND parent, AppWindowState& state);

} // namespace ovtr::win32
