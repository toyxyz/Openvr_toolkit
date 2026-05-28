#include "platform/win32/WindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppState.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

bool handleMainWindowMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return false;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    ScreenToClient(hwnd, &point);
    const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
    int wheelSteps = wheelDelta / WHEEL_DELTA;
    if (wheelSteps == 0) {
        wheelSteps = wheelDelta > 0 ? 1 : -1;
    }

    const DeviceListLayout deviceListLayout = deviceListLayoutForClient(state, clientWidth, clientHeight);
    if (deviceListLayout.valid && PtInRect(&deviceListLayout.boxRect, point)) {
        state->deviceListScrollOffset -= wheelSteps * 3;
        clampDeviceListScroll(*state, deviceListLayout.visibleItemCount);
        InvalidateRect(hwnd, &deviceListLayout.boxRect, FALSE);
        return true;
    }

    const MarkerListLayout markerListLayout = markerListLayoutForClient(state, clientWidth, clientHeight);
    if (markerListLayout.valid && PtInRect(&markerListLayout.boxRect, point)) {
        state->markerListScrollOffset -= wheelSteps * 3;
        clampMarkerListScroll(*state, markerListLayout.visibleItemCount);
        InvalidateRect(hwnd, &markerListLayout.boxRect, FALSE);
        return true;
    }

    const RECT debugInfoRect = debugInfoRectForClient(state, clientWidth, clientHeight);
    if (state->debugMonitorVisible && PtInRect(&debugInfoRect, point)) {
        const int visibleLineCount = visibleDebugLogLineCount(debugInfoRect);
        state->debugInfoScrollOffset -= wheelSteps * 3;
        clampDebugInfoScroll(*state, visibleLineCount);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    const RECT messagesRect = debugMessagesRectForClient(state, clientWidth, clientHeight);
    if (state->debugMonitorVisible && PtInRect(&messagesRect, point)) {
        const int visibleLineCount = visibleDebugLogLineCount(messagesRect);
        state->debugLogScrollOffset += wheelSteps * 3;
        clampDebugLogScroll(*state, visibleLineCount);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    return false;
}

} // namespace ovtr::win32
