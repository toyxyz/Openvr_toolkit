#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/ViewportMath.h"

#include <array>

namespace ovtr::win32 {

struct AppViewportState;

void drawBodySkeletonBoxes3D(
    AppViewportState& viewportState,
    const ProfileSkeletonJoints& joints,
    float heightMeters,
    Vec3 offset,
    const std::array<MappingVirtualTarget, kMappingSlotCount>* targets = nullptr
);

} // namespace ovtr::win32
