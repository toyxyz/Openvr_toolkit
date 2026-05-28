#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool handleTopBarClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleViewportControlClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleDeviceToggleClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleOriginStepperClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleDebugResizeClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleDeviceSplitterClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleDeviceListClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleMarkerListClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleDebugToggleClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);

} // namespace ovtr::win32
