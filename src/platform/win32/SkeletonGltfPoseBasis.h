#pragma once

#include "platform/win32/ProfileSkeleton.h"

#include <array>

namespace ovtr::win32 {

struct SkeletonGltfBasis {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

std::array<Vec3, kProfileSkeletonJointCount> exportSkeletonGltfWorldPositions(const ProfileSkeletonJoints& joints);
ProfileSkeletonJoints exportSkeletonGltfJoints(const ProfileSkeletonJoints& joints);
SkeletonGltfBasis skeletonGltfExportBasisFor(const ProfileSkeletonJoints& joints, int joint) noexcept;
SkeletonGltfBasis alignSkeletonGltfBasisRoll(SkeletonGltfBasis basis, SkeletonGltfBasis rest) noexcept;
SkeletonGltfBasis skeletonGltfBasisFromYAndXHint(
    Vec3 y,
    Vec3 xHint,
    SkeletonGltfBasis fallback
) noexcept;
std::array<float, 4> skeletonGltfQuaternionFromBasis(const SkeletonGltfBasis& basis) noexcept;
std::array<float, 4> closestSkeletonGltfRoll(
    SkeletonGltfBasis basis,
    const std::array<float, 4>& previous
) noexcept;
std::array<float, 4> skeletonGltfSwingBetween(Vec3 from, Vec3 to) noexcept;
Vec3 skeletonGltfPrimaryBoneDirection(const ProfileSkeletonJoints& joints, int joint) noexcept;

} // namespace ovtr::win32
