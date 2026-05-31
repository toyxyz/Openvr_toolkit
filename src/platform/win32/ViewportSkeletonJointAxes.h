#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/ProfileSkeleton.h"

#include <array>

namespace ovtr::win32 {

void drawSkeletonJointAxes3D(
    const ProfileSkeletonJoints& joints,
    Vec3 offset,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets = nullptr
);

} // namespace ovtr::win32
