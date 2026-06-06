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

inline bool toggleMappingActorSelectionAtIndex(AppProfileState& state, const std::size_t actorIndex)
{
    if (actorIndex >= state.mappingActors.size()) {
        return false;
    }
    const std::uint32_t actorId = state.mappingActors[actorIndex].id;
    if (state.selectedMappingActorId == actorId) {
        state.selectedMappingActorId = 0;
        state.selectedMappingOffsetSlot = -1;
        state.mappingEditStepDropdownOpen = false;
        state.mappingEditOffsetPresetDropdownOpen = false;
        return true;
    }
    const MappingActor& actor = state.mappingActors[actorIndex];
    state.selectedMappingActorId = actorId;
    state.mappingActorName = effectiveMappingActorName(actor);
    state.profile = actor.profile;
    state.mappingSkeletonColor = actor.skeletonColor;
    state.mappingSkeletonColorCustomized = true;
    state.mappingDeviceRuntimeIndices = actor.calibrated
        ? actor.calibration.runtimeIndices
        : actor.mappingDeviceRuntimeIndices;
    state.mappingFingerRuntimeIndices = actor.mappingFingerRuntimeIndices;
    state.selectedMappingOffsetSlot = -1;
    state.mappingEditStepDropdownOpen = false;
    state.mappingEditOffsetPresetDropdownOpen = false;
    return true;
}

inline MappingActor* selectedMappingActor(AppProfileState& state) noexcept
{
    for (MappingActor& actor : state.mappingActors) {
        if (actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    return nullptr;
}

inline const MappingActor* selectedMappingActor(const AppProfileState& state) noexcept
{
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    return nullptr;
}

inline void resetMappingActorCalibration(MappingActor& actor) noexcept
{
    actor.calibrated = false;
    actor.calibration = MappingCalibrationData{};
    actor.liveJoints = buildProfileSkeletonJoints(actor.profile);
    actor.liveSkeletonPose = makeRestSkeletonPose(actor.liveJoints);
    actor.liveJointsValid = false;
    actor.liveTrackingLost = false;
    actor.liveLimited = false;
    actor.liveVirtualTargets = {};
    actor.liveDebugPoles = {};
    actor.livePoleDirections = {};
    actor.livePoleDirectionValid = {};
    actor.livePoleTargets = {};
    actor.livePoleTargetValid = {};
    actor.liveFingerJointsValid = {};
}

inline void syncSelectedMappingActorFromControls(AppProfileState& state)
{
    MappingActor* actor = selectedMappingActor(state);
    if (!actor) {
        return;
    }
    actor->profile = state.profile;
    actor->name = state.mappingActorName.empty() ? state.profile.name : state.mappingActorName;
    actor->skeletonColor = state.mappingSkeletonColor;
    actor->mappingDeviceRuntimeIndices = state.mappingDeviceRuntimeIndices;
    actor->mappingFingerRuntimeIndices = state.mappingFingerRuntimeIndices;
}

} // namespace ovtr::win32
