#include "platform/win32/SkeletonGltfHierarchy.h"

#include <cstddef>

namespace ovtr::win32 {

bool isSkeletonGltfExportedJoint(const int joint) noexcept
{
    return joint != kProfileJointHeadTopEnd;
}

bool isSkeletonGltfSkinJoint(const int joint) noexcept
{
    return isSkeletonGltfExportedJoint(joint) &&
        joint != kProfileJointHips;
}

int skeletonGltfParentIndex(const ProfileSkeletonJoints& rest, const int joint) noexcept
{
    if (joint == kProfileJointSpine) {
        return kProfileJointHips;
    }
    if (joint == kProfileJointSpine1 ||
        joint == kProfileJointLeftUpLeg ||
        joint == kProfileJointRightUpLeg) {
        return kProfileJointSpine;
    }
    return rest[static_cast<std::size_t>(joint)].parentIndex;
}

} // namespace ovtr::win32
