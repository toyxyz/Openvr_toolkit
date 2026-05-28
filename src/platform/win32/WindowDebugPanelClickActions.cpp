#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleDebugResizeClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT debugResizeRect = debugResizeRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&debugResizeRect, point)) {
        return false;
    }

    state.debugResizeDragging = true;
    state.debugMonitorHeight = activeDebugMonitorHeight(&state, clientHeight);
    SetCapture(hwnd);
    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

bool handleDebugToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT debugButtonRect = debugButtonRectForClient(clientWidth, clientHeight);
    if (!PtInRect(&debugButtonRect, point)) {
        return false;
    }

    state.debugMonitorVisible = !state.debugMonitorVisible;
    appendDebugLog(
        state,
        state.debugMonitorVisible ? L"Debug monitor opened" : L"Debug monitor closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

} // namespace ovtr::win32
