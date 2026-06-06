#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

void applyEstimatedChestTarget(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    std::array<MappingVirtualTarget, kMappingSlotCount>& targets
);

} // namespace ovtr::win32
