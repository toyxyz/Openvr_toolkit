#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

inline constexpr const wchar_t* kMainWindowClassName = L"OpenVRTrackerRecorderBootstrapWindow";
inline constexpr const wchar_t* kViewportWindowClassName = L"OpenVRTrackerRecorderGLViewport";

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void createViewportChild(HWND hwnd, HINSTANCE instance, AppWindowState& state);
void loadAppConfiguration(AppWindowState& state);
void destroyAppWindowState(HWND hwnd);

} // namespace ovtr::win32
