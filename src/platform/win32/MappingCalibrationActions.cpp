#include "platform/win32/MappingCalibrationActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/RecordingStartActions.h"

#include <cstddef>
#include <mutex>

namespace ovtr::win32 {
namespace {

MappingActor* selectedActor(AppWindowState& state) noexcept
{
    for (MappingActor& actor : state.mappingActors) {
        if (actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    return nullptr;
}

void refreshCalibrationUi(HWND hwnd, const AppWindowState& state)
{
    InvalidateRect(hwnd, nullptr, FALSE);
    if (state.glWindow) {
        InvalidateRect(state.glWindow, nullptr, FALSE);
        UpdateWindow(state.glWindow);
    }
}

void startRecordingForCalibration(HWND hwnd, AppWindowState& state)
{
    if (!state.startRecordingOnCalibration) {
        return;
    }
    if (state.loadedSessionActive) {
        appendDebugLog(state, L"Calibration recording start blocked: close loaded session first");
        return;
    }

    bool canceledDelay = false;
    bool canStart = false;
    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        canceledDelay = state.recordingDelayActive;
        state.recordingDelayActive = false;
        const ovtr::RecorderState recorderState = state.recorder.state();
        canStart = recorderState == ovtr::RecorderState::Idle ||
            recorderState == ovtr::RecorderState::Error;
    }
    if (!canStart) {
        appendDebugLog(state, L"Calibration recording start skipped: recorder is busy");
        return;
    }
    if (canceledDelay) {
        appendDebugLog(state, L"Recording delay canceled for calibration start");
    }
    startRecordingNow(hwnd, state);
}

} // namespace

void calibrateSelectedMappingActor(HWND hwnd, AppWindowState& state)
{
    MappingActor* actor = selectedActor(state);
    if (!actor) {
        MessageBoxW(hwnd, L"Select an actor to calibrate.", L"Mapping", MB_OK | MB_ICONWARNING);
        return;
    }

    const MappingCalibrationStatus status = captureMappingActorCalibration(
        *actor,
        state.mappingDeviceRuntimeIndices,
        state.poses,
        state.originEnabled,
        state.originOffset,
        state.originRotationDegrees,
        state.mappingArmSoftIkStrength,
        state.mappingLegSoftIkStrength,
        state.mappingPinHandTargets,
        state.mappingPinFootTargets
    );
    if (!status.success) {
        MessageBoxW(hwnd, status.message.c_str(), L"Mapping", MB_OK | MB_ICONWARNING);
        appendDebugLog(state, L"Mapping calibration failed: " + status.message);
        return;
    }

    actor->mappingFingerRuntimeIndices = state.mappingFingerRuntimeIndices;
    appendDebugLog(state, L"Mapping actor calibrated: " + actor->profile.name);
    // Temporary CSV diagnostics remain in MappingCalibrationPoseDebugLog.* for future reuse.
    startRecordingForCalibration(hwnd, state);
    refreshCalibrationUi(hwnd, state);
}

} // namespace ovtr::win32
