#pragma once

#include "platform/win32/AppProfileState.h"

#include <filesystem>

namespace ovtr::win32 {

bool startMappingCalibrationTargetOffsetDebugLog(
    const MappingActor& actor,
    const std::filesystem::path& path
);
void appendMappingCalibrationTargetOffsetDebugLog(const MappingActor& actor);

} // namespace ovtr::win32
