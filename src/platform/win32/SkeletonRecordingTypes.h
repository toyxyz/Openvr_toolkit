#pragma once

#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonPose.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct SkeletonRecordingFrame {
    double timeSeconds = 0.0;
    SkeletonPose pose;
};

struct SkeletonRecordingClip {
    bool active = false;
    std::uint32_t actorId = 0;
    BodyProfile profile;
    std::wstring actorName;
    std::vector<SkeletonRecordingFrame> frames;
};

} // namespace ovtr::win32
