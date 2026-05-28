#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/win32/FrameUpdate.h"
#include "platform/win32/RuntimeStatus.h"
#include "platform/win32/Win32AppWindow.h"
#include "platform/win32/WindowLayout.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    if (!ovtr::win32::registerBootstrapWindowClasses(instance)) {
        return 1;
    }

    HWND hwnd = ovtr::win32::createMainWindow(instance);
    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);
    ovtr::win32::layoutChildWindows(hwnd);
    ovtr::win32::refreshStatus(hwnd, true);
    ovtr::win32::refreshPoseAndViewport(hwnd);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    return ovtr::win32::runMessageAndFrameLoop(hwnd);
}
