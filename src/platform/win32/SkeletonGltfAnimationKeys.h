#pragma once

#include "platform/win32/SkeletonPose.h"

#include <vector>

namespace ovtr::win32 {

std::vector<float> continuousJointRotationKeys(const std::vector<SkeletonPose>& poses, int joint);

} // namespace ovtr::win32
