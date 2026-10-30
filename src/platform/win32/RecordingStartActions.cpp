#include "platform/win32/RecordingStartActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/ImportedSceneActions.h"
#include "platform/win32/RecordingStartPlan.h"
#include "platform/win32/SessionMappingSnapshot.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/WindowLayout.h"
#include "recording/SamplingScheduler.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace ovtr::win32 {
namespace {

constexpr double kTargetViewportFps = 90.0;

std::string localTimestampForPath()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &currentTime);

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y_%m_%d_%H%M%S");
    return stream.str();
}

std::string utcTimestampIso()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm utcTime{};
    gmtime_s(&utcTime, &currentTime);

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

const MappingActor* skeletonRecordingActor(const AppWindowState& state) noexcept
{
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.calibrated && actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.calibrated) {
            return &actor;
        }
    }
    return nullptr;
}

void completeRecordingStart(
    HWND hwnd,
    AppWindowState& state,
    RecordingStartPlan plan,
    const bool recordVideo
)
{
    bool recordingStarted = false;
    std::string recordingError;
    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        state.recordingDelayActive = false;
        state.exportStatusMessage.clear();
        state.currentSessionFolder = plan.sessionFolder;
        state.recordingDroppedFrames = 0;
        state.recordingScheduler = ovtr::SamplingScheduler(kTargetViewportFps);
        state.recordingScheduler.reset(state.recordingStart);
        state.skeletonRecording = SkeletonRecordingClip{};

        recordingStarted = state.recorder.start(plan.options);
        if (!recordingStarted) {
            state.recordingError = state.recorder.lastError();
            recordingError = state.recordingError;
        } else if (const MappingActor* actor = skeletonRecordingActor(state)) {
            beginSkeletonRecording(state.skeletonRecording, *actor);
        }
    }
    if (!recordingStarted && recordVideo) {
        std::string finalizeError;
        if (!state.videoCapture->endRecording(finalizeError)) {
            appendDebugLog(
                state, "Video cleanup after recording start failure failed: " + finalizeError);
        }
    }

    appendDebugLog(state, "Starting recording: " + plan.sessionFolder.string());
    if (recordingStarted) {
        appendDebugLog(state, L"Recording started");
        if (recordVideo) {
            state.videoStatusMessage = "Video recording.";
            appendDebugLog(state, L"Session video capture started");
        }
        std::string snapshotError;
        if (saveSessionMappingSnapshot(
                state, state.devices, plan.sessionFolder, snapshotError)) {
            appendDebugLog(state, L"Session mapping snapshot saved");
        } else {
            appendDebugLog(
                state, "Session mapping snapshot save failed: " + snapshotError);
        }
        appendDebugLog(
            state,
            state.skeletonRecording.active
                ? L"Skeleton GLB capture started"
                : L"Skeleton GLB capture skipped: no calibrated Mapping actor");
        startImportedGlbPlaybackForRecording(hwnd, state);
    } else {
        appendDebugLog(state, "Recording start failed: " + recordingError);
    }
    invalidateStatusPanel(hwnd);
}

} // namespace

void startRecordingNow(HWND hwnd, AppWindowState& state)
{
    if (state.pendingVideoRecordingStart) {
        state.videoStatusMessage = "Video recording preparation is already running.";
        invalidateStatusPanel(hwnd);
        return;
    }
    RecordingStartPlan plan = makeRecordingStartPlan(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state),
        activeSessionDirectoryPath(state),
        recordingSessionIdForName(state.sessionName, localTimestampForPath()),
        utcTimestampIso(),
        kTargetViewportFps
    );
    const bool cameraSelected = !state.selectedVideoCameraLink.empty();
    if (cameraSelected &&
        (!state.videoCapture ||
         state.videoCapture->previewState() != VideoPreviewState::Ready)) {
        const std::string message =
            state.videoCapture &&
            state.videoCapture->previewState() == VideoPreviewState::Opening
                ? "Wait for the selected camera to finish opening."
                : "The selected camera is not ready. Select it again or choose Disable.";
        state.recordingError = message;
        state.videoStatusMessage = message;
        appendDebugLog(state, "Recording blocked: " + message);
        invalidateStatusPanel(hwnd);
        return;
    }
    const bool recordVideo = cameraSelected;
    if (recordVideo) {
        plan.options.session.videoPath = "video.mp4";
    }

    state.recordingDelayActive = false;
    std::string recordingError;
    if (recordVideo && !state.videoCapture->beginRecording(
            plan.sessionFolder / plan.options.session.videoPath, recordingError)) {
        state.recordingError = recordingError;
        appendDebugLog(state, "Video recording start failed: " + recordingError);
        invalidateStatusPanel(hwnd);
        return;
    }
    if (recordVideo) {
        state.pendingVideoRecordingStart = std::move(plan);
        state.videoStatusMessage = "Preparing video recording...";
        appendDebugLog(state, L"Preparing session video encoder");
        invalidateStatusPanel(hwnd);
        return;
    }
    state.recordingStart = std::chrono::steady_clock::now();
    completeRecordingStart(hwnd, state, std::move(plan), false);
}

void pollPreparedRecordingStart(HWND hwnd, AppWindowState& state)
{
    if (!state.pendingVideoRecordingStart || !state.videoCapture) return;
    if (state.videoCapture->previewState() != VideoPreviewState::Ready) {
        std::string cleanupError;
        state.videoCapture->endRecording(cleanupError);
        state.pendingVideoRecordingStart.reset();
        state.recordingError = "The camera stopped while preparing video recording.";
        if (!cleanupError.empty()) {
            state.recordingError += " Cleanup failed: " + cleanupError;
        }
        state.videoStatusMessage = state.recordingError;
        appendDebugLog(state, "Video recording preparation canceled: " +
            state.recordingError);
        invalidateStatusPanel(hwnd);
        return;
    }
    const VideoRecordingPreparationState preparation =
        state.videoCapture->preparationState();
    if (preparation == VideoRecordingPreparationState::Pending) return;
    if (preparation == VideoRecordingPreparationState::Failed) {
        const std::string error = state.videoCapture->preparationError();
        state.pendingVideoRecordingStart.reset();
        state.recordingError = error.empty()
            ? "Preparing video recording failed."
            : error;
        state.videoStatusMessage = state.recordingError;
        appendDebugLog(
            state, "Video recording preparation failed: " + state.recordingError);
        invalidateStatusPanel(hwnd);
        return;
    }
    if (preparation != VideoRecordingPreparationState::Ready) return;

    state.recordingStart = std::chrono::steady_clock::now();
    std::string activationError;
    if (!state.videoCapture->activatePreparedRecording(activationError)) {
        if (activationError == "Waiting for the first camera frame.") {
            state.videoStatusMessage = activationError;
            return;
        }
        state.pendingVideoRecordingStart.reset();
        state.recordingError = activationError;
        state.videoStatusMessage = activationError;
        appendDebugLog(
            state, "Activating prepared video recording failed: " + activationError);
        invalidateStatusPanel(hwnd);
        return;
    }
    RecordingStartPlan plan = std::move(*state.pendingVideoRecordingStart);
    state.pendingVideoRecordingStart.reset();
    completeRecordingStart(hwnd, state, std::move(plan), true);
}

} // namespace ovtr::win32
