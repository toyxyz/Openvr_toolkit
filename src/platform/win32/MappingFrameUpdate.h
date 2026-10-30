#pragma once

#include "platform/win32/AppDebugUiState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppStreamingState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/VmcStreamingPose.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <chrono>

namespace ovtr::win32 {

// Application work runs once per sampled display frame, never per camera pane or paint.
inline void updateMappingActorsForFrame(AppProfileState& profiles, const AppRuntimeState& runtime,
    const AppOriginState& origin, AppRecordingState& recording, AppStreamingState& streaming,
    AppDebugUiState& diagnostics)
{
    const bool selectedCalibrated = std::any_of(profiles.mappingActors.begin(), profiles.mappingActors.end(),
        [&](const MappingActor& actor) { return actor.calibrated && actor.id == profiles.selectedMappingActorId; });
    const auto now = std::chrono::steady_clock::now();
    bool streamedActor = false;
    for (MappingActor& actor : profiles.mappingActors) {
        if (!actor.calibrated) continue;
        updateCalibratedMappingActorJoints(actor, runtime.poses, origin.originEnabled,
            origin.originOffset, origin.originRotationDegrees);
        if (!actor.liveJointsValid) continue;
        recordSkeletonFrameIfRecording(recording, actor, now);
        if (streamedActor || (selectedCalibrated && actor.id != profiles.selectedMappingActorId)) continue;
        std::string error;
        if (sendMappingActorVmcPose(streaming, actor, error)) {
            streaming.vmcSendErrorLogged = false;
        } else if (!streaming.vmcSendErrorLogged) {
            appendDebugLog(diagnostics, L"VMC streaming send failed: " + widen(error));
            streaming.vmcSendErrorLogged = true;
        }
        streamedActor = true;
    }
}

} // namespace ovtr::win32
