#pragma once

#include "platform/win32/ProfileSkeleton.h"

#include <array>

namespace ovtr::win32 {

void applyVmcLimbSpacing(
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    float armSpacingDegrees,
    float legSpacingDegrees
) noexcept;

} // namespace ovtr::win32
