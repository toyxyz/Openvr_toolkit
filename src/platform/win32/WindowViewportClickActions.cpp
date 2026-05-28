#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleViewportControlClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const ViewportControlLayout viewportControls = viewportControlLayoutForClient(
        &state,
        clientWidth,
        clientHeight
    );
    if (handleImportedAnimationControlClick(hwnd, state, viewportControls, point)) {
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.quadViewButtonRect, point)) {
        state.quadViewEnabled = !state.quadViewEnabled;
        state.orbitDragging = false;
        state.panDragging = false;
        state.activeDragPane = ViewportPaneKind::None;
        InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
        if (state.glWindow) {
            InvalidateRect(state.glWindow, nullptr, FALSE);
        }
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.recordButtonRect, point)) {
        toggleRecording(hwnd);
        InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
        return true;
    }
    return false;
}

} // namespace ovtr::win32
