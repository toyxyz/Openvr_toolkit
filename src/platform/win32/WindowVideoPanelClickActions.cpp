#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/VideoCameraRefresh.h"
#include "platform/win32/WindowLayout.h"

#include <algorithm>

namespace ovtr::win32 {
bool handleVideoToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
) {
    const RECT button = videoToggleButtonRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&button, point)) {
        return false;
    }
    state.videoPanelVisible = !state.videoPanelVisible;
    if (state.videoPanelVisible) {
        state.devicePanelVisible = false;
        state.sessionPanelVisible = false;
        state.streamingPanelVisible = false;
        if (state.videoCameras.empty()) {
            requestVideoCameraRefresh(state);
        }
    }
    state.splitterDragging = false;
    appendDebugLog(state, state.videoPanelVisible ? L"Video panel opened" : L"Video panel closed");
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

bool handleVideoPanelClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
) {
    const VideoPanelLayout layout = videoPanelLayoutForClient(&state, clientWidth, clientHeight);
    if (!layout.valid) {
        return false;
    }
    if (PtInRect(&layout.refreshButtonRect, point)) {
        if (state.videoFinalizationPending) {
            state.videoStatusMessage = "Camera list cannot change while finalizing video.";
        } else if (state.videoCapture && state.videoCapture->recording()) {
            state.videoStatusMessage = "Camera list cannot change while recording.";
        } else {
            requestVideoCameraRefresh(state);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    const int index = videoCameraIndexFromPoint(
        layout, point, static_cast<int>(state.videoCameras.size()) + 1);
    if (index < 0) {
        return PtInRect(&layout.boxRect, point) != FALSE;
    }
    if (state.videoFinalizationPending ||
        (state.videoCapture && state.videoCapture->recording())) {
        state.videoStatusMessage = state.videoFinalizationPending
            ? "Camera cannot change while finalizing video."
            : "Camera cannot change while recording.";
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (index == 0) {
        if (requestVideoCameraDisable(state)) {
            state.selectedVideoCamera = -1;
            state.selectedVideoCameraLink.clear();
            state.selectedVideoCameraName.clear();
            appendDebugLog(state, L"Video camera disable requested");
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    const int cameraIndex = index - 1;
    const VideoCameraInfo& camera = state.videoCameras[static_cast<std::size_t>(cameraIndex)];
    if (!state.videoCaptureSwitch.valid() &&
        camera.symbolicLink == state.selectedVideoCameraLink &&
        state.videoCapture &&
        state.videoCapture->previewState() == VideoPreviewState::Ready) {
        state.videoStatusMessage = "Preview running.";
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }
    if (requestVideoCameraSwitch(state, camera)) {
        state.selectedVideoCamera = cameraIndex;
        state.selectedVideoCameraLink = camera.symbolicLink;
        state.selectedVideoCameraName = camera.displayName;
        appendDebugLog(state, L"Video camera switch requested: " + camera.displayName);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
