#pragma once

#include "platform/win32/SkeletonPose.h"

namespace ovtr::win32 {

void stabilizeSkeletonFingerRolls(const ProfileSkeletonJoints& rest, SkeletonPose& pose);

} // namespace ovtr::win32
