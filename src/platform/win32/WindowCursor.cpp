#include "platform/win32/WindowInput.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

bool setHandCursor()
{
    SetCursor(LoadCursor(nullptr, IDC_HAND));
    return true;
}

} // namespace

bool handleMainWindowSetCursor(HWND hwnd, LPARAM lparam)
{
    if (LOWORD(lparam) != HTCLIENT) {
        return false;
    }

    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return false;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    POINT point{};
    GetCursorPos(&point);
    ScreenToClient(hwnd, &point);

    const RECT settingRect = topBarSettingRectForClient(clientWidth, clientHeight);
    if (PtInRect(&settingRect, point)) {
        return setHandCursor();
    }
    const RECT fileRect = topBarFileRectForClient(clientWidth, clientHeight);
    if (PtInRect(&fileRect, point)) {
        return setHandCursor();
    }

    const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
        state,
        clientWidth,
        clientHeight
    );
    if (viewportControls.animationValid &&
        (PtInRect(&viewportControls.firstFrameButtonRect, point) ||
         PtInRect(&viewportControls.playPauseButtonRect, point) ||
         PtInRect(&viewportControls.lastFrameButtonRect, point) ||
         PtInRect(&viewportControls.timelineRect, point) ||
         PtInRect(&viewportControls.closeButtonRect, point))) {
        return setHandCursor();
    }

    const OriginStepperButton originButton = originStepperButtonFromPoint(
        state,
        clientWidth,
        clientHeight,
        point
    );
    if (originButton.valid) {
        return setHandCursor();
    }

    const RECT deviceButtonRect = deviceToggleButtonRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&deviceButtonRect, point)) {
        return setHandCursor();
    }

    const RECT debugResizeRect = debugResizeRectForClient(state, clientWidth, clientHeight);
    if (state->debugResizeDragging || PtInRect(&debugResizeRect, point)) {
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        return true;
    }

    const RECT splitterRect = splitterRectForClient(state, clientWidth, clientHeight);
    if (state->devicePanelVisible && (state->splitterDragging || PtInRect(&splitterRect, point))) {
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }

    return false;
}

} // namespace ovtr::win32
