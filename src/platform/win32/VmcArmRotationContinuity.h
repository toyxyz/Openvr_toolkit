#pragma once

#include "platform/win32/VmcLegRotationContinuity.h"

namespace ovtr::win32 {

void applyVmcArmRotationsWithContinuity(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    VmcLegRotationContinuity& continuity
) noexcept;

} // namespace ovtr::win32
