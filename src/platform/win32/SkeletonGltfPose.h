#pragma once

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"

#include <vector>

namespace ovtr::win32 {

SkeletonPose makeSkeletonGltfExportPose(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose,
    bool alignRollToRest = false
);

std::vector<SkeletonPose> makeSkeletonGltfExportPoses(
    const ProfileSkeletonJoints& rest,
    const std::vector<SkeletonPose>& poses
);

} // namespace ovtr::win32
