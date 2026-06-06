#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

void applyMappingLegRollHints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const std::array<Vec3, kProfileSkeletonJointCount>* previousSideAxes = nullptr
) noexcept;

} // namespace ovtr::win32
