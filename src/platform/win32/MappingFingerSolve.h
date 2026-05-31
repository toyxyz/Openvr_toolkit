#pragma once

#include "platform/win32/AppProfileState.h"
#include "vr/IVRProvider.h"

namespace ovtr::win32 {

void applyRestHandFingerJoints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const MappingTransform& wristTarget,
    ProfileSkeletonHandSide side
) noexcept;

void updateMappingActorFingerJoints(
    MappingActor& actor,
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const ovtr::PosePollResult& poses,
    bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets
);

} // namespace ovtr::win32
