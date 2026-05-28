#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool handleCameraKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleOriginKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleRefreshKeyDown(HWND hwnd, WPARAM wparam);
bool handleRecordingKeyDown(HWND hwnd, WPARAM wparam);

} // namespace ovtr::win32
