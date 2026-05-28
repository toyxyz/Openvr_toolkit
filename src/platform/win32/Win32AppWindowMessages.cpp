#include "platform/win32/Win32AppWindowMessages.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/FrameUpdate.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/RuntimeStatus.h"
#include "platform/win32/Win32AppWindowInternal.h"
#include "platform/win32/WindowLayout.h"

#include <memory>

namespace ovtr::win32 {

bool handleMainWindowCreate(HWND hwnd, LPARAM lparam)
{
    auto ownedState = std::make_unique<AppWindowState>();
    AppWindowState* state = ownedState.get();
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    ownedState.release();

    const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
    createViewportChild(hwnd, createStruct->hInstance, *state);
    appendDebugLog(*state, L"Application window created");
    loadAppConfiguration(*state);
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    refreshStatus(hwnd);
    startPoseSamplingWorker(hwnd, *state);
    refreshPoseAndViewport(hwnd);
    return true;
}

void handleMainWindowDestroy(HWND hwnd, const UINT_PTR timerId)
{
    KillTimer(hwnd, timerId);
    destroyAppWindowState(hwnd);
    PostQuitMessage(0);
}

} // namespace ovtr::win32
