#pragma once

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonGltfPoseBasis.h"

#include <array>

namespace ovtr::win32 {

struct VmcLegRotationContinuity;

SkeletonGltfBasis vmcFingerOutputPoseBasisForJoint(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    ProfileSkeletonHandSide side,
    int joint
) noexcept;

void applyVmcFingerOutputRotations(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    VmcLegRotationContinuity* continuity = nullptr
) noexcept;

} // namespace ovtr::win32
