#include "platform/win32/WindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppState.h"
#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/Layout.h"
#include "platform/win32/PanelLayoutMetrics.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

constexpr int kStatusBarHeight = 36;

} // namespace

bool handleMainWindowMouseMove(HWND hwnd, LPARAM lparam)
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

    if (state->importedSceneTimelineDragging) {
        const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
            state,
            clientWidth,
            clientHeight
        );
        seekImportedGlbFromTimeline(hwnd, *state, viewportControls.timelineRect, point);
        SetCursor(LoadCursor(nullptr, IDC_HAND));
        return true;
    }
    if (state->loadedSessionTimelineDragging) {
        const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
            state,
            clientWidth,
            clientHeight
        );
        seekLoadedSessionFromTimeline(*state, viewportControls.timelineRect, point);
        sampleLoadedSessionFrame(*state);
        if (state->glWindow) {
            renderViewport(state->glWindow);
        }
        InvalidateRect(hwnd, &viewportControls.animationBarRect, FALSE);
        SetCursor(LoadCursor(nullptr, IDC_HAND));
        return true;
    }

    if (state->debugResizeDragging) {
        const int statusBarTop = clientHeight > kStatusBarHeight ? clientHeight - kStatusBarHeight : 0;
        const int newDebugMonitorHeight = clampDebugMonitorHeightForClient(
            statusBarTop - point.y,
            clientHeight
        );
        if (state->debugMonitorHeight != newDebugMonitorHeight) {
            state->debugMonitorHeight = newDebugMonitorHeight;
            layoutChildWindows(hwnd);
            invalidateWindowLayout(hwnd);
        }
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        return true;
    }

    if (state->splitterDragging) {
        const int newLeftPanelWidth = clampLeftPanelWidthForClient(point.x, clientWidth);
        if (state->leftPanelWidth != newLeftPanelWidth) {
            state->leftPanelWidth = newLeftPanelWidth;
            layoutChildWindows(hwnd);
            invalidateWindowLayout(hwnd);
        }
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }

    if (state->profileSplitterDragging) {
        const int railLeft = clientWidth - kPanelProfileToggleRailWidth;
        const int newProfileWidth = clampProfilePanelWidthForClient(railLeft - point.x, clientWidth);
        if (state->profilePanelWidth != newProfileWidth) {
            state->profilePanelWidth = newProfileWidth;
            layoutChildWindows(hwnd);
            invalidateWindowLayout(hwnd);
        }
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }

    const RECT splitterRect = splitterRectForClient(state, clientWidth, clientHeight);
    if (state->devicePanelVisible && PtInRect(&splitterRect, point)) {
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }
    const RECT profileSplitterRect = profileSplitterRectForClient(state, clientWidth, clientHeight);
    if ((state->profilePanelVisible || state->mappingPanelVisible || state->editPanelVisible) &&
        PtInRect(&profileSplitterRect, point)) {
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        return true;
    }
    const RECT debugResizeRect = debugResizeRectForClient(state, clientWidth, clientHeight);
    if (PtInRect(&debugResizeRect, point)) {
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        return true;
    }

    return false;
}
} // namespace ovtr::win32
