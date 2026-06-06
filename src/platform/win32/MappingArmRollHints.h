#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

void applyMappingArmRollHints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const std::array<Vec3, kProfileSkeletonJointCount>* previousSideAxes
) noexcept;

} // namespace ovtr::win32
