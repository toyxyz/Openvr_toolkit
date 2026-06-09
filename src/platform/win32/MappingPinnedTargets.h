#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/SkeletonPose.h"

namespace ovtr::win32 {

void applyPinnedMappingTargets(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    SkeletonPose& pose
);

} // namespace ovtr::win32
