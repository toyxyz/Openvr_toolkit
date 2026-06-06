#pragma once

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonRecordingTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

bool writeSkeletonGltfPoseCompareLog(
    const SkeletonRecordingClip& clip,
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& bindPose,
    const std::vector<SkeletonPose>& gltfPoses,
    const std::filesystem::path& glbPath,
    std::string& error
);

} // namespace ovtr::win32
