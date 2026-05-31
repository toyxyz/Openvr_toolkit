#pragma once

#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

struct MappingCoreSolveResult {
    bool limited = false;
};

MappingCoreSolveResult solveMappingCoreJoints(
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    ProfileSkeletonJoints& out
);

} // namespace ovtr::win32
