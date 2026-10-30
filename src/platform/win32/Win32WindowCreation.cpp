#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/win32/Win32AppWindow.h"
#include "platform/win32/UiWindowFrame.h"

#include "platform/win32/Win32AppWindowInternal.h"

namespace ovtr::win32 {

HWND createMainWindow(HINSTANCE instance)
{
    HWND hwnd = CreateWindowExW(
        0,
        kMainWindowClassName,
        L"toyxyz OpenVR Toolkit",
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
    if (hwnd) applyDarkWindowFrame(hwnd);
    return hwnd;
}

} // namespace ovtr::win32
