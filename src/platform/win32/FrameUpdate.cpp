#include "platform/win32/FrameUpdate.h"
#include "platform/win32/CombinedPlayback.h"
#include "platform/win32/PerformanceCapture.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/MappingFrameUpdate.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/RecordingStartActions.h"
#include "platform/win32/RecordingVideoFinalization.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/VideoCameraRefresh.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/ViewportRedraw.h"
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

void updateVideoRuntimeStatus(AppWindowState& state)
{
    std::string error;
    const bool sessionExpectsVideo =
        state.loadedSessionActive && !state.loadedSession.videoPath.empty();
    if (sessionExpectsVideo && state.sessionVideoPlayback) {
        error = state.sessionVideoPlayback->runtimeError();
        if (error.empty() &&
            state.sessionVideoPlayback->state() == VideoPlaybackState::Opening) {
            state.videoStatusMessage = "Opening session video...";
        } else if (error.empty() &&
                   state.sessionVideoPlayback->state() == VideoPlaybackState::Ready &&
                   state.videoStatusMessage == "Opening session video...") {
            state.videoStatusMessage = "Session video ready.";
        }
    } else if (state.videoCapture) {
        error = state.videoCapture->runtimeError();
        if (state.videoCapture->previewState() == VideoPreviewState::Failed &&
            !state.selectedVideoCameraLink.empty() &&
            !state.videoCaptureSwitch.valid() &&
            !state.videoCameraChangeQueued) {
            state.selectedVideoCamera = -1;
            state.selectedVideoCameraLink.clear();
            state.selectedVideoCameraName.clear();
        } else if (error.empty() &&
                   state.videoCapture->previewState() == VideoPreviewState::Ready &&
                   state.videoStatusMessage == "Opening selected camera...") {
            state.videoStatusMessage = "Preview running.";
        }
    }
    if (!error.empty() && error != state.videoStatusMessage) {
        state.videoStatusMessage = error;
        appendDebugLog(state, "Video error: " + error);
    }
    if (state.videoCapture) {
        const std::uint64_t dropped = state.videoCapture->droppedRecordingFrames();
        if (dropped != state.reportedVideoDroppedFrames) {
            state.reportedVideoDroppedFrames = dropped;
            if (dropped > 0) {
                state.videoStatusMessage =
                    "Video encoder overloaded: " + std::to_string(dropped) + " frame(s) dropped.";
                appendDebugLog(state, state.videoStatusMessage);
            }
        }
    }
}

void invalidateChangedVideoFrame(HWND hwnd, AppWindowState& state)
{
    static HWND trackedWindow = nullptr;
    static std::uint64_t lastSequence = 0;
    static bool lastWasPlayback = false;
    if (!state.videoPanelVisible) {
        return;
    }
    if (trackedWindow != hwnd) {
        trackedWindow = hwnd;
        lastSequence = 0;
        lastWasPlayback = false;
    }
    const bool playback = state.loadedSessionActive &&
        !state.loadedSession.videoPath.empty();
    const VideoFrameSnapshot frame = playback
        ? (state.sessionVideoPlayback
            ? state.sessionVideoPlayback->latestFrame()
            : VideoFrameSnapshot{})
        : (state.videoCapture ? state.videoCapture->latestFrame() : VideoFrameSnapshot{});
    if (!frame || (frame->sequence == lastSequence && playback == lastWasPlayback)) {
        return;
    }
    lastSequence = frame->sequence;
    lastWasPlayback = playback;
    RECT client{};
    GetClientRect(hwnd, &client);
    const VideoPanelLayout layout = videoPanelLayoutForClient(
        &state, client.right - client.left, client.bottom - client.top);
    if (layout.valid) {
        InvalidateRect(hwnd, &layout.previewRect, FALSE);
    }
}

} // namespace

void refreshPoseAndViewport(HWND hwnd)
{
    PerformanceScope trace("frame_update");
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    pollVideoCameraTasks(hwnd, *state);
    pollRecordingVideoFinalization(hwnd, *state);
    pollPreparedRecordingStart(hwnd, *state);

    const bool animationWasPlaying = state->importedScenePlaying || state->loadedSessionPlaying;
    updateDelayedRecordingStart(hwnd, *state);
    updateCombinedPlayback(*state);
    if (state->loadedSessionActive) {
        sampleLoadedSessionFrame(*state);
        if (state->sessionVideoPlayback && state->sessionVideoPlayback->opened()) {
            state->sessionVideoPlayback->requestFrame(state->loadedSessionPlaybackSeconds);
        }
    } else {
        state->poses = copyLatestPoseSnapshot(*state);
    }
    updateVideoRuntimeStatus(*state);
    invalidateChangedVideoFrame(hwnd, *state);
    updateCameraDeviceLock(*state);
    updateMappingActorsForFrame(*state, *state, *state, *state, *state, *state);

    if (state->glWindow && viewportNeedsRedraw(hwnd, *state)) {
        renderViewport(state->glWindow);
    }
    if (animationWasPlaying || state->importedScenePlaying || state->loadedSessionPlaying) {
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
