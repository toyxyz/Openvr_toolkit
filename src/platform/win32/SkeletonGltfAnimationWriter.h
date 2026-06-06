#pragma once

#include "platform/win32/ProfileSkeleton.h"

#include <array>
#include <iosfwd>

namespace ovtr::win32 {

bool isSkeletonGltfRotationAnimatedJoint(int joint) noexcept;
bool isSkeletonGltfTranslationAnimatedJoint(int joint) noexcept;

void writeSkeletonGltfAnimation(
    std::ostream& out,
    int timeAccessor,
    int rootTranslationAccessor,
    const std::array<int, kProfileSkeletonJointCount>& rotationAccessors,
    const std::array<int, kProfileSkeletonJointCount>& translationAccessors
);

} // namespace ovtr::win32
