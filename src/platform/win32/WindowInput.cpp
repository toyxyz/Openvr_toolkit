#include "platform/win32/WindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppState.h"
#include "platform/win32/WindowClickActions.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

bool handleMainWindowLButtonDoubleClick(HWND hwnd, LPARAM lparam)
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
    return handleMainWindowDoubleClickAtPoint(hwnd, *state, clientWidth, clientHeight, point);
}

bool handleMainWindowLButtonDown(HWND hwnd, LPARAM lparam)
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
    return handleMainWindowLeftClickAtPoint(hwnd, *state, clientWidth, clientHeight, point);
}

} // namespace ovtr::win32
