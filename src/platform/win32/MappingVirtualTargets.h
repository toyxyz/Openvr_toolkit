#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "vr/IVRProvider.h"

namespace ovtr::win32 {

struct MappingVirtualTargetBuildResult {
    bool success = false;
    int failedSlot = -1;
    bool trackingLost = false;
};

MappingVirtualTargetBuildResult buildMappingVirtualTargets(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    std::array<MappingVirtualTarget, kMappingSlotCount>& outTargets
);

MappingVirtualTargetBuildResult buildMappingVirtualTargetsWithFallback(
    const MappingCalibrationData& calibration,
    const ovtr::PosePollResult& poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& fallbackTargets,
    std::array<MappingVirtualTarget, kMappingSlotCount>& outTargets
);

} // namespace ovtr::win32
