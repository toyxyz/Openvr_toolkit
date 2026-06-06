#pragma once

#include "platform/win32/ProfileSkeleton.h"

namespace ovtr::win32 {

bool isSkeletonGltfExportedJoint(int joint) noexcept;
bool isSkeletonGltfSkinJoint(int joint) noexcept;
int skeletonGltfParentIndex(const ProfileSkeletonJoints& rest, int joint) noexcept;

} // namespace ovtr::win32
