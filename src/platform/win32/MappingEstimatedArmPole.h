#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

void applyEstimatedArmPoleTargets(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& solvedCore,
    const std::array<Vec3, kMappingPoleCount>& previousPoleDirections,
    const std::array<bool, kMappingPoleCount>& previousPoleDirectionValid,
    const std::array<Vec3, kMappingPoleCount>& previousPoleTargets,
    const std::array<bool, kMappingPoleCount>& previousPoleTargetValid,
    std::array<MappingVirtualTarget, kMappingSlotCount>& targets
);

} // namespace ovtr::win32
