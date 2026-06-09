#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/SessionActions.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"

#include <mutex>

namespace ovtr::win32 {
namespace {

bool handleLoadedSessionControlClick(
    HWND hwnd,
    AppWindowState& state,
    const ViewportControlLayout& layout,
    const POINT point
)
{
    if (!state.loadedSessionActive || !layout.animationValid) {
        return false;
    }

    if (PtInRect(&layout.firstFrameButtonRect, point)) {
        setLoadedSessionPlaybackSeconds(state, 0.0);
    } else if (PtInRect(&layout.playPauseButtonRect, point)) {
        toggleLoadedSessionPlayback(state);
    } else if (PtInRect(&layout.lastFrameButtonRect, point)) {
        setLoadedSessionPlaybackSeconds(state, loadedSessionDurationSeconds(state));
    } else if (PtInRect(&layout.closeButtonRect, point)) {
        closeLoadedSessionFolder(hwnd, state);
        return true;
    } else if (PtInRect(&layout.timelineRect, point)) {
        state.loadedSessionTimelineDragging = true;
        seekLoadedSessionFromTimeline(state, layout.timelineRect, point);
        SetCapture(hwnd);
    } else {
        return false;
    }

    sampleLoadedSessionFrame(state);
    InvalidateRect(hwnd, &layout.animationBarRect, FALSE);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    return true;
}

} // namespace

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
    if (handleLoadedSessionControlClick(hwnd, state, viewportControls, point)) {
        return true;
    }
    if (handleImportedAnimationControlClick(hwnd, state, viewportControls, point)) {
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.quadViewButtonRect, point)) {
        toggleQuadView(state);
        InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
        if (state.glWindow) {
            InvalidateRect(state.glWindow, nullptr, FALSE);
        }
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.showTextButtonRect, point)) {
        state.deviceLabelsVisible = !state.deviceLabelsVisible;
        InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
        if (state.glWindow) {
            InvalidateRect(state.glWindow, nullptr, FALSE);
        }
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.showModelButtonRect, point)) {
        state.trackedDevicesVisible = !state.trackedDevicesVisible;
        InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
        if (state.glWindow) {
            InvalidateRect(state.glWindow, nullptr, FALSE);
        }
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.smoothButtonRect, point)) {
        {
            std::lock_guard<std::mutex> lock(state.realtimeSmoothingMutex);
            state.realtimeSmoothingEnabled = !state.realtimeSmoothingEnabled;
            state.realtimePoseSmoother.reset();
        }
        saveStreamingSettingsConfig(state);
        InvalidateRect(hwnd, &viewportControls.barRect, FALSE);
        if (state.glWindow) {
            InvalidateRect(state.glWindow, nullptr, FALSE);
        }
        return true;
    }
    if (viewportControls.valid && PtInRect(&viewportControls.lockButtonRect, point)) {
        state.cameraDeviceLockEnabled = !state.cameraDeviceLockEnabled;
        resetCameraDeviceLockAnchor(state);
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
