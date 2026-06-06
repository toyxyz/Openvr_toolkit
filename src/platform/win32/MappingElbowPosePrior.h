#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

Vec3 sampleElbowPosePreferredDirection(
    const MappingTransform& chest,
    Vec3 wrist,
    bool left
) noexcept;

} // namespace ovtr::win32
