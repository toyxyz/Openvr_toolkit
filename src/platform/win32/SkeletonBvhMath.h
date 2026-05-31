#pragma once

#include "platform/win32/ProfileSkeleton.h"

#include <array>

namespace ovtr::win32 {

struct SkeletonBvhRotationFrame {
    std::array<float, 3> rootPositionCm{};
    std::array<std::array<float, 3>, kProfileSkeletonJointCount> eulerDegrees{};
};

SkeletonBvhRotationFrame makeSkeletonBvhRotationFrame(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& pose
);

} // namespace ovtr::win32
