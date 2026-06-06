#pragma once

#include "platform/win32/AppProfileState.h"

#include <string>

namespace ovtr::win32 {

bool startMappingCalibrationPoseDebugLog(const MappingActor& actor, std::wstring& message);
void appendMappingCalibrationPoseDebugLog(const MappingActor& actor);

} // namespace ovtr::win32
