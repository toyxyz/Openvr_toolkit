#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/ConfigTypes.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/ViewportMath.h"

#include <array>

namespace ovtr::win32 {

struct AppViewportState;

void drawBodySkeletonBoxes3D(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& joints,
    float heightMeters,
    Vec3 offset,
    RgbColor color,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets = nullptr
);

void drawBodySkeletonBoxesFromPose3D(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const SkeletonPose& pose,
    float heightMeters,
    Vec3 offset,
    RgbColor color
);

} // namespace ovtr::win32
