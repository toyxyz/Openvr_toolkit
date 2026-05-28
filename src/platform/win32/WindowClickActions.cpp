#include "platform/win32/WindowClickActions.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Layout.h"
#include "platform/win32/OriginEditor.h"
#include "platform/win32/SessionEditor.h"
#include "platform/win32/WindowClickActionSections.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleMainWindowDoubleClickAtPoint(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const ViewportControlLayout viewportControlLayout =
        viewportControlLayoutForClient(&state, clientWidth, clientHeight);
    if (viewportControlLayout.valid && PtInRect(&viewportControlLayout.sessionBoxRect, point)) {
        showSessionEditor(hwnd, state);
        return true;
    }

    const OriginPanelLayout originLayout = originPanelLayoutForClient(&state, clientWidth, clientHeight);
    const OriginStepperButton originButton = originStepperButtonFromPoint(
        &state,
        clientWidth,
        clientHeight,
        point
    );
    if (originButton.valid) {
        applyOriginStepperButton(hwnd, state, originButton);
        return true;
    }
    if (originLayout.valid && PtInRect(&originLayout.boxRect, point)) {
        showOriginEditor(hwnd, state);
        return true;
    }

    return false;
}

bool handleMainWindowLeftClickAtPoint(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    return handleTopBarClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleViewportControlClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleDeviceToggleClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleOriginStepperClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleDebugResizeClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleDeviceSplitterClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleMarkerListClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleDeviceListClick(hwnd, state, clientWidth, clientHeight, point) ||
        handleDebugToggleClick(hwnd, state, clientWidth, clientHeight, point);
}

} // namespace ovtr::win32
