#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/ProfileModel.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/ConfigTypes.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ovtr::win32 {

enum class ProfileEditKind {
    None,
    Name,
    Height,
    Measurement
};

struct ProfileEditTarget {
    ProfileEditKind kind = ProfileEditKind::None;
    int measurementIndex = -1;
};

struct MappingActor {
    std::uint32_t id = 0;
    BodyProfile profile;
    std::array<std::uint32_t, kMappingSlotCount> mappingDeviceRuntimeIndices =
        defaultMappingDeviceRuntimeIndices();
    RgbColor skeletonColor{255, 255, 255};
    bool calibrated = false;
    MappingCalibrationData calibration;
    SkeletonPose liveSkeletonPose;
    ProfileSkeletonJoints liveJoints{};
    bool liveJointsValid = false;
    bool liveTrackingLost = false;
    bool liveLimited = false;
    std::array<MappingVirtualTarget, kMappingSlotCount> liveVirtualTargets{};
    std::array<MappingDebugPole, kMappingPoleCount> liveDebugPoles{};
    std::array<Vec3, kMappingPoleCount> livePoleDirections{};
    std::array<bool, kMappingPoleCount> livePoleDirectionValid{};
    std::array<Vec3, kMappingPoleCount> livePoleTargets{};
    std::array<bool, kMappingPoleCount> livePoleTargetValid{};
    std::array<bool, 2> liveFingerJointsValid{};
};

struct AppProfileState {
    bool profilePanelVisible = false;
    bool mappingPanelVisible = false;
    bool editPanelVisible = false;
    int profilePanelWidth = 0;
    bool profileSplitterDragging = false;
    bool profilePreviewEnabled = false;
    int profileScrollOffset = 0;
    int mappingScrollOffset = 0;
    int mappingDropdownSlot = -1;
    bool mappingProfileDropdownOpen = false;
    bool mappingPresetDropdownOpen = false;
    std::wstring mappingPresetName;
    RgbColor mappingSkeletonColor{255, 255, 255};
    bool mappingSkeletonColorCustomized = false;
    std::wstring mappingNameEditSnapshot;
    HWND mappingNameEditWindow = nullptr;
    WNDPROC mappingNameEditOriginalProc = nullptr;
    std::vector<MappingActor> mappingActors;
    std::uint32_t nextMappingActorId = 1;
    std::uint32_t selectedMappingActorId = 0;
    int mappingActorScrollOffset = 0;
    int selectedMappingOffsetSlot = -1;
    int mappingEditOffsetScrollOffset = 0;
    float mappingEditOffsetStepMeters = 0.001f;
    bool mappingEditStepDropdownOpen = false;
    bool mappingEditOffsetPresetDropdownOpen = false;
    std::wstring mappingEditOffsetPresetName;
    std::wstring mappingEditOffsetPresetNameSnapshot;
    HWND mappingEditOffsetPresetNameEditWindow = nullptr;
    WNDPROC mappingEditOffsetPresetNameEditOriginalProc = nullptr;
    float mappingArmSoftIkStrength = kDefaultMappingArmSoftIkStrength;
    float mappingLegSoftIkStrength = kDefaultMappingLegSoftIkStrength;
    std::array<std::uint32_t, kMappingSlotCount> mappingDeviceRuntimeIndices =
        defaultMappingDeviceRuntimeIndices();
    BodyProfile profile;
    BodyProfile profileEditSnapshot;
    bool profileEditSnapshotValid = false;
    HWND profileEditWindow = nullptr;
    WNDPROC profileEditOriginalProc = nullptr;
    ProfileEditTarget profileEditTarget;
};

inline void disableProfilePreview(AppProfileState& state) noexcept
{
    state.profilePreviewEnabled = false;
}

inline void syncMappingSoftIkStrengthsToActors(AppProfileState& state) noexcept
{
    for (MappingActor& actor : state.mappingActors) {
        if (!actor.calibrated) {
            continue;
        }
        actor.calibration.armSoftIkStrength = state.mappingArmSoftIkStrength;
        actor.calibration.legSoftIkStrength = state.mappingLegSoftIkStrength;
    }
}

inline void resetMappingActorLiveContinuity(MappingActor& actor) noexcept
{
    actor.liveJointsValid = false;
    actor.livePoleDirectionValid = {};
    actor.livePoleTargetValid = {};
    actor.liveFingerJointsValid = {};
}

inline void resetMappingActorsLiveContinuity(AppProfileState& state) noexcept
{
    for (MappingActor& actor : state.mappingActors) {
        resetMappingActorLiveContinuity(actor);
    }
}

} // namespace ovtr::win32
