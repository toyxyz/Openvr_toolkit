#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool handleCameraKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleSideMenuVisibilityKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleTrackedDeviceVisibilityKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleDeviceLabelKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleQuadViewKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleMappingCalibrationKeyDown(HWND hwnd, AppWindowState* state, WPARAM wparam);
bool handleRefreshKeyDown(HWND hwnd, WPARAM wparam);
bool handleRecordingKeyDown(HWND hwnd, WPARAM wparam);

} // namespace ovtr::win32
