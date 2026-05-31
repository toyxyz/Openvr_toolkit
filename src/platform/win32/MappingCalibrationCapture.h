#pragma once

#include "platform/win32/AppProfileState.h"
#include "vr/IVRProvider.h"

namespace ovtr::win32 {

MappingCalibrationStatus captureMappingActorCalibration(
    MappingActor& actor,
    const std::array<std::uint32_t, kMappingSlotCount>& mappingRuntimeIndices,
    const ovtr::PosePollResult& poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    float armSoftIkStrength = kDefaultMappingSoftIkStrength,
    float legSoftIkStrength = kDefaultMappingSoftIkStrength
);

} // namespace ovtr::win32
