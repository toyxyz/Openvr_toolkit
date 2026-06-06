#pragma once

#include "data/SkeletalSyntheticPose.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/ProfileSkeleton.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {

struct MappingHandBasis {
    Vec3 forward{};
    Vec3 side{};
    Vec3 spread{};
    bool valid = false;
};

struct MappingFingerAlignment {
    MappingTransform sourceToRoot{};
    Vec3 sourceWristRoot{};
    MappingHandBasis sourceBasis{};
    MappingHandBasis restBasis{};
    bool valid = false;
};

inline Vec3 crossHandBasis(const Vec3 left, const Vec3 right) noexcept
{
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

inline Vec3 projectOffHandBasis(const Vec3 value, const Vec3 axis) noexcept
{
    return subMappingVec3(value, scaleMappingVec3(axis, dotMappingVec3(value, axis)));
}

inline Vec3 negHandBasis(const Vec3 value) noexcept
{
    return {-value.x, -value.y, -value.z};
}

inline Vec3 preferredPalmSideForHand(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonHandSide side,
    const int restWrist
) noexcept {
    (void)side;
    const Vec3 hint = rest[static_cast<std::size_t>(restWrist)].sideHint;
    return hint;
}

inline Vec3 addBasisVector(const Vec3 total, const Vec3 axis, const float amount) noexcept
{
    return addMappingVec3(total, scaleMappingVec3(axis, amount));
}

inline void orientHandBasisSide(MappingHandBasis& basis, const Vec3 preferredSide) noexcept
{
    if (lengthMappingVec3(preferredSide) > 0.00001f && dotMappingVec3(basis.side, preferredSide) < 0.0f) {
        basis.side = negHandBasis(basis.side);
    }
}

inline void mirrorRightHandSourceBasis(
    MappingHandBasis& basis,
    const ProfileSkeletonHandSide side
) noexcept {
    if (side == ProfileSkeletonHandSide::Right) {
        basis.side = negHandBasis(basis.side);
    }
}

inline Vec3 fallbackSpreadForHandBasis(const Vec3 forward) noexcept
{
    const float zAlignment = dotMappingVec3(forward, Vec3{0.0f, 0.0f, 1.0f});
    const Vec3 preferred = zAlignment > -0.8f && zAlignment < 0.8f
        ? Vec3{0.0f, 0.0f, 1.0f}
        : Vec3{1.0f, 0.0f, 0.0f};
    return normalizeMappingVec3Or(projectOffHandBasis(preferred, forward), Vec3{0.0f, 0.0f, 1.0f});
}

inline MappingHandBasis makeHandBasisFromPoints(
    const Vec3 wrist,
    const Vec3 middleTip,
    const Vec3 indexBase,
    const Vec3 pinkyBase,
    const Vec3 preferredSide
) noexcept {
    MappingHandBasis basis;
    basis.forward = normalizeMappingVec3Or(subMappingVec3(middleTip, wrist), {});
    if (lengthMappingVec3(basis.forward) <= 0.00001f) {
        return basis;
    }
    basis.spread = normalizeMappingVec3Or(
        projectOffHandBasis(subMappingVec3(indexBase, pinkyBase), basis.forward),
        fallbackSpreadForHandBasis(basis.forward)
    );
    basis.side = normalizeMappingVec3Or(crossHandBasis(basis.spread, basis.forward), {});
    if (lengthMappingVec3(preferredSide) > 0.00001f && dotMappingVec3(basis.side, preferredSide) < 0.0f) {
        basis.side = negHandBasis(basis.side);
    }
    basis.valid = lengthMappingVec3(basis.side) > 0.00001f;
    return basis;
}

inline Vec3 pointInRoot(
    const MappingTransform& sourceToRoot,
    const std::array<MappingTransform, ovtr::kSkeletalHandBoneCount>& transforms,
    const int boneIndex
) noexcept {
    return transformMappingPoint(sourceToRoot, transforms[static_cast<std::size_t>(boneIndex)].position);
}

inline MappingTransform translationOnlySourceRoot(const MappingTransform& sourceRoot) noexcept
{
    MappingTransform sourceToRoot;
    sourceToRoot.position = negHandBasis(sourceRoot.position);
    return sourceToRoot;
}

inline MappingFingerAlignment makeMappingFingerAlignment(
    const std::array<MappingTransform, ovtr::kSkeletalHandBoneCount>& transforms,
    const std::array<bool, ovtr::kSkeletalHandBoneCount>& valid,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonHandSide side
) noexcept {
    constexpr int kRootBone = 0;
    constexpr int kWristBone = 1;
    constexpr int kIndexBaseBone = 6;
    constexpr int kMiddleBaseBone = 11;
    constexpr int kPinkyBaseBone = 21;
    if (!valid[kWristBone] || !valid[kIndexBaseBone] || !valid[kMiddleBaseBone] || !valid[kPinkyBaseBone]) {
        return {};
    }
    const int sourceRoot = valid[kRootBone] ? kRootBone : kWristBone;
    MappingFingerAlignment alignment;
    alignment.sourceToRoot = translationOnlySourceRoot(transforms[static_cast<std::size_t>(sourceRoot)]);
    alignment.sourceWristRoot = pointInRoot(alignment.sourceToRoot, transforms, kWristBone);
    alignment.sourceBasis = makeHandBasisFromPoints(
        alignment.sourceWristRoot,
        pointInRoot(alignment.sourceToRoot, transforms, kMiddleBaseBone),
        pointInRoot(alignment.sourceToRoot, transforms, kIndexBaseBone),
        pointInRoot(alignment.sourceToRoot, transforms, kPinkyBaseBone),
        {}
    );
    mirrorRightHandSourceBasis(alignment.sourceBasis, side);

    const int restWrist = profileHandRootJoint(side);
    alignment.restBasis = makeHandBasisFromPoints(
        rest[static_cast<std::size_t>(restWrist)].positionMeters,
        rest[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Middle, 1))].positionMeters,
        rest[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Index, 1))].positionMeters,
        rest[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Pinky, 1))].positionMeters,
        preferredPalmSideForHand(rest, side, restWrist)
    );
    orientHandBasisSide(alignment.restBasis, preferredPalmSideForHand(rest, side, restWrist));
    alignment.valid = alignment.sourceBasis.valid && alignment.restBasis.valid;
    return alignment;
}

inline Vec3 alignSkeletalHandPoint(
    const MappingFingerAlignment& alignment,
    const Vec3 sourcePoint
) noexcept {
    const Vec3 sourceRoot = transformMappingPoint(alignment.sourceToRoot, sourcePoint);
    const Vec3 local = subMappingVec3(sourceRoot, alignment.sourceWristRoot);
    const float forward = dotMappingVec3(local, alignment.sourceBasis.forward);
    const float side = dotMappingVec3(local, alignment.sourceBasis.side);
    const float spread = dotMappingVec3(local, alignment.sourceBasis.spread);
    Vec3 result{};
    result = addBasisVector(result, alignment.restBasis.forward, forward);
    result = addBasisVector(result, alignment.restBasis.side, side);
    result = addBasisVector(result, alignment.restBasis.spread, spread);
    return result;
}

} // namespace ovtr::win32
