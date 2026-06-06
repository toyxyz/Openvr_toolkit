#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"

#include <array>

namespace ovtr::win32 {

void drawSkeletonJointAxes3D(
    const ProfileSkeletonJoints& joints,
    Vec3 offset,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets = nullptr
);

void drawSkeletonPoseJointAxes3D(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const SkeletonPose& pose,
    Vec3 offset
);

} // namespace ovtr::win32
