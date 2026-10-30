#include "platform/win32/RecordingVideoFinalization.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/RecordingSessionList.h"
#include "platform/win32/RecordingUiActions.h"
#include "platform/win32/VideoCaptureService.h"
#include "platform/win32/WindowLayout.h"
#include "recording/SessionManifestRecovery.h"

#include <filesystem>
#include <sstream>
#include <utility>

namespace ovtr::win32 {
namespace {

bool preserveTrackerOnlySession(AppWindowState& state, std::string& outError)
{
    if (state.currentSessionFolder.empty()) {
        outError = "Current session folder is unavailable.";
        return false;
    }
    if (!ovtr::removeVideoReferenceFromSessionManifest(
            state.currentSessionFolder / "manifest.json", outError)) {
        return false;
    }
    state.recorder.discardSessionVideoReference();
    invalidateRecordingSessionListCache(state);
    outError.clear();
    return true;
}

void finishSuccessfulRecording(HWND hwnd, AppWindowState& state)
{
    std::wostringstream stream;
    stream << L"Recording stopped: frames " << state.pendingStoppedFrameCount
           << L", dropped " << state.pendingStoppedDroppedFrames;
    appendDebugLog(state, stream.str());
    state.videoStatusMessage = "Video finalized.";
    invalidateRecordingSessionListCache(state);
    const bool exportAfterRecording = state.pendingExportAfterRecording;
    state.videoFinalizationPending = false;
    if (exportAfterRecording) {
        exportCurrentSession(hwnd);
    } else {
        appendDebugLog(state, L"Export after recording disabled");
    }
}

void finishFailedRecording(
    AppWindowState& state,
    const std::string& videoError,
    const bool videoFinalizationFailed
)
{
    std::string error = state.pendingTrackerStopSucceeded
        ? videoError
        : state.pendingTrackerStopError;
    if (!state.pendingTrackerStopSucceeded && !videoError.empty()) {
        error += "; video finalization failed: " + videoError;
    }
    if (state.pendingTrackerStopSucceeded && videoFinalizationFailed) {
        std::string recoveryError;
        if (preserveTrackerOnlySession(state, recoveryError)) {
            error += " Tracker data was preserved without video.";
        } else {
            error += "; tracker-only recovery failed: " + recoveryError;
        }
    }
    if (error.empty()) error = "Unknown recording finalization failure.";
    state.recordingError = error;
    state.videoStatusMessage = error;
    state.videoFinalizationPending = false;
    appendDebugLog(state, "Recording stop failed: " + error);
}

} // namespace

bool requestRecordingVideoFinalization(
    HWND hwnd,
    AppWindowState& state,
    const bool trackerStopSucceeded,
    std::string trackerStopError,
    const std::uint64_t stoppedFrameCount,
    const std::uint64_t stoppedDroppedFrames,
    std::string& outError
) {
    if (!state.videoCapture ||
        !state.videoCapture->requestEndRecording(outError)) {
        if (outError.empty()) {
            outError = state.videoCapture
                ? "Video finalization could not be started."
                : "Video capture service is unavailable.";
        }
        if (trackerStopSucceeded) {
            std::string recoveryError;
            if (preserveTrackerOnlySession(state, recoveryError)) {
                outError += " Tracker data was preserved without video.";
            } else {
                outError += "; tracker-only recovery failed: " + recoveryError;
            }
        }
        return false;
    }
    state.videoFinalizationPending = true;
    state.pendingTrackerStopSucceeded = trackerStopSucceeded;
    state.pendingTrackerStopError = std::move(trackerStopError);
    state.pendingStoppedFrameCount = stoppedFrameCount;
    state.pendingStoppedDroppedFrames = stoppedDroppedFrames;
    state.pendingExportAfterRecording = state.exportAfterRecording;
    state.videoStatusMessage = "Finalizing video...";
    appendDebugLog(state, L"Finalizing session video in background");
    invalidateStatusPanel(hwnd);
    return true;
}

void pollRecordingVideoFinalization(HWND hwnd, AppWindowState& state)
{
    if (!state.videoFinalizationPending || !state.videoCapture) return;
    const VideoFinalizationState finalization = state.videoCapture->finalizationState();
    if (finalization == VideoFinalizationState::Idle ||
        finalization == VideoFinalizationState::Pending) {
        return;
    }
    const std::string videoError = finalization == VideoFinalizationState::Failed
        ? state.videoCapture->finalizationError()
        : std::string{};
    if (finalization == VideoFinalizationState::Succeeded &&
        state.pendingTrackerStopSucceeded) {
        finishSuccessfulRecording(hwnd, state);
    } else {
        finishFailedRecording(
            state,
            videoError,
            finalization == VideoFinalizationState::Failed);
    }
    invalidateStatusPanel(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

} // namespace ovtr::win32
