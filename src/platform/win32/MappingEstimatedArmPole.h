#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

void applyEstimatedArmPoleTargets(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& solvedCore,
    std::array<Vec3, kMappingPoleCount>& previousPoleLocalDirections,
    std::array<bool, kMappingPoleCount>& previousPoleLocalDirectionValid,
    std::array<Vec3, kMappingPoleCount>& previousHandLocalPositions,
    std::array<bool, kMappingPoleCount>& previousHandLocalPositionValid,
    std::array<MappingVirtualTarget, kMappingSlotCount>& targets
);

} // namespace ovtr::win32
