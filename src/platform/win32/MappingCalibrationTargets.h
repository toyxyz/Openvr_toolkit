#pragma once

#include "platform/win32/MappingCalibrationModel.h"
#include "platform/win32/ProfileModel.h"

namespace ovtr::win32 {

std::array<MappingTransform, kMappingSlotCount> mappingCalibrationRestTargets(
    const BodyProfile& profile
);
Vec3 mappingCalibrationArmPoleTargetPosition(const ProfileSkeletonJoints& joints, bool left) noexcept;

} // namespace ovtr::win32
