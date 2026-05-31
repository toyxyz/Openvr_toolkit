#pragma once

#include "platform/win32/SkeletonRecordingTypes.h"

#include <filesystem>
#include <string>

namespace ovtr::win32 {

bool exportSkeletonRecordingToBvh(
    const SkeletonRecordingClip& clip,
    const std::filesystem::path& outputPath,
    std::string& error
);

} // namespace ovtr::win32
