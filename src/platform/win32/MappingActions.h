#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppProfileState.h"

#include <cstddef>

namespace ovtr::win32 {

struct AppWindowState;

bool handleMappingPanelClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool handleMappingPanelDoubleClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);
bool resetMappingActorAtIndex(HWND hwnd, AppWindowState& state, std::size_t actorIndex);
bool deleteMappingActorAtIndex(HWND hwnd, AppWindowState& state, std::size_t actorIndex);

inline void resetMappingActorCalibration(MappingActor& actor) noexcept
{
    actor.calibrated = false;
    actor.calibration = MappingCalibrationData{};
    actor.liveJoints = buildProfileSkeletonJoints(actor.profile);
    actor.liveJointsValid = false;
    actor.liveTrackingLost = false;
    actor.liveLimited = false;
    actor.liveVirtualTargets = {};
    actor.liveDebugPoles = {};
    actor.livePoleDirections = {};
    actor.livePoleDirectionValid = {};
    actor.liveFingerJointsValid = {};
}

} // namespace ovtr::win32
