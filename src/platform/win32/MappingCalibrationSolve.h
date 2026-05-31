#pragma once

#include "platform/win32/AppProfileState.h"
#include "vr/IVRProvider.h"

namespace ovtr::win32 {

bool updateCalibratedMappingActorJoints(
    MappingActor& actor,
    const ovtr::PosePollResult& poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
);

} // namespace ovtr::win32
