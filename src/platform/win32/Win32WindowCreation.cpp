#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/win32/Win32AppWindow.h"

#include "platform/win32/Win32AppWindowInternal.h"

namespace ovtr::win32 {

HWND createMainWindow(HINSTANCE instance)
{
    return CreateWindowExW(
        0,
        kMainWindowClassName,
        L"OpenVR Tracker Recorder",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1480,
        860,
        nullptr,
        nullptr,
        instance,
        nullptr
    );
}

} // namespace ovtr::win32
