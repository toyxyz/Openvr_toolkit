#include "platform/win32/WindowKeyboardSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/FrameUpdate.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/RuntimeStatus.h"

namespace ovtr::win32 {

bool handleRefreshKeyDown(HWND hwnd, const WPARAM wparam)
{
    if (wparam != VK_F5) {
        return false;
    }
    refreshStatus(hwnd, true);
    refreshPoseAndViewport(hwnd);
    return true;
}

bool handleDeviceLabelKeyDown(HWND hwnd, AppWindowState* state, const WPARAM wparam)
{
    if (wparam != VK_F2) {
        return false;
    }
    if (state) {
        state->deviceLabelsVisible = !state->deviceLabelsVisible;
        refreshPoseAndViewport(hwnd);
    }
    return true;
}

bool handleTrackedDeviceVisibilityKeyDown(HWND hwnd, AppWindowState* state, const WPARAM wparam)
{
    if (wparam != VK_F1) {
        return false;
    }
    if (state) {
        state->trackedDevicesVisible = !state->trackedDevicesVisible;
        refreshPoseAndViewport(hwnd);
    }
    return true;
}

bool handleQuadViewKeyDown(HWND hwnd, AppWindowState* state, const WPARAM wparam)
{
    if (wparam != 'Q') {
        return false;
    }
    if (state) {
        toggleQuadView(*state);
        refreshPoseAndViewport(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    return true;
}

bool handleRecordingKeyDown(HWND hwnd, const WPARAM wparam)
{
    if (wparam != VK_SPACE) {
        return false;
    }
    toggleRecording(hwnd);
    return true;
}

} // namespace ovtr::win32
