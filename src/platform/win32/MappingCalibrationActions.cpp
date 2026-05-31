#include "platform/win32/MappingCalibrationActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/MappingCalibrationCapture.h"

#include <cstddef>

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
        state.mappingLegSoftIkStrength
    );
    if (!status.success) {
        MessageBoxW(hwnd, status.message.c_str(), L"Mapping", MB_OK | MB_ICONWARNING);
        appendDebugLog(state, L"Mapping calibration failed: " + status.message);
        return;
    }

    appendDebugLog(state, L"Mapping actor calibrated: " + actor->profile.name);
    refreshCalibrationUi(hwnd, state);
}

} // namespace ovtr::win32
