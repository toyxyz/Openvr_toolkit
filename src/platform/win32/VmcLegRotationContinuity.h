#pragma once

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"

#include <array>

namespace ovtr::win32 {

struct VmcLegRotationContinuity {
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> previousWorld{};
    std::array<bool, kProfileSkeletonJointCount> valid{};
};

void resetVmcLegRotationContinuity(VmcLegRotationContinuity& continuity) noexcept;

std::array<std::array<float, 4>, kProfileSkeletonJointCount> makeVmcLocalRotationsWithContinuity(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const SkeletonPose& pose,
    VmcLegRotationContinuity& continuity
);

} // namespace ovtr::win32
