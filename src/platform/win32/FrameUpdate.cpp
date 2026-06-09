#include "platform/win32/FrameUpdate.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

const ovtr::PoseSample* selectedValidPose(const AppWindowState& state) noexcept
{
    if (state.selectedDeviceRuntimeIndex == kNoSelectedRuntimeIndex) {
        return nullptr;
    }
    for (const ovtr::PoseSample& pose : state.poses.poses) {
        const bool usable = pose.runtimeIndex == state.selectedDeviceRuntimeIndex &&
            (pose.flags & ovtr::PoseFlagDeviceConnected) != 0 &&
            (pose.flags & ovtr::PoseFlagPoseValid) != 0;
        if (usable) {
            return &pose;
        }
    }
    return nullptr;
}

void updateCameraDeviceLock(AppWindowState& state) noexcept
{
    if (!state.cameraDeviceLockEnabled) {
        resetCameraDeviceLockAnchor(state);
        return;
    }

    const ovtr::PoseSample* pose = selectedValidPose(state);
    if (!pose) {
        resetCameraDeviceLockAnchor(state);
        return;
    }

    if (!state.cameraDeviceLockHasPosition ||
        state.cameraDeviceLockRuntimeIndex != pose->runtimeIndex) {
        state.cameraDeviceLockRuntimeIndex = pose->runtimeIndex;
        state.cameraDeviceLockLastPosition = pose->position;
        state.cameraDeviceLockHasPosition = true;
        return;
    }

    state.cameraPanX += pose->position[0] - state.cameraDeviceLockLastPosition[0];
    state.cameraPanY += pose->position[1] - state.cameraDeviceLockLastPosition[1];
    state.cameraPanZ += pose->position[2] - state.cameraDeviceLockLastPosition[2];
    state.cameraDeviceLockLastPosition = pose->position;
}

} // namespace

void refreshPoseAndViewport(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    updateDelayedRecordingStart(hwnd, *state);
    updateImportedScenePlayback(*state);
    updateLoadedSessionPlayback(*state);
    if (state->loadedSessionActive) {
        sampleLoadedSessionFrame(*state);
    } else {
        state->poses = copyLatestPoseSnapshot(*state);
    }
    updateCameraDeviceLock(*state);

    if (state->glWindow) {
        renderViewport(state->glWindow);
    }
    if (state->importedSceneLoaded || state->loadedSessionActive) {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        const ViewportControlLayout controls = viewportControlLayoutForClient(
            state,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top
        );
        if (controls.animationValid) {
            InvalidateRect(hwnd, &controls.animationBarRect, FALSE);
        }
    }
}

} // namespace ovtr::win32
