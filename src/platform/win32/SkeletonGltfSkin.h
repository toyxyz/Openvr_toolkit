#pragma once

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"

#include <vector>

namespace ovtr::win32 {

std::vector<float> makeSkeletonInverseBindMatrices(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& bindPose
);

} // namespace ovtr::win32
