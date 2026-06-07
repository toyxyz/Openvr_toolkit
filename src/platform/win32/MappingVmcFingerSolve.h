#pragma once

#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationModel.h"
#include "vr/IVRProvider.h"

#include <array>

namespace ovtr::win32 {

bool mappingVmcFingerInputEnabledForSide(
    const MappingActor& actor,
    ProfileSkeletonHandSide side
) noexcept;

bool applyVmcFingerHandFromPoses(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const ovtr::PosePollResult& poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    ProfileSkeletonHandSide side,
    const MappingTransform& targetWrist
);

} // namespace ovtr::win32
