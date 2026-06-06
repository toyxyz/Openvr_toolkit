#include "platform/win32/SkeletonFingerRoll.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

struct FingerBasis {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

Vec3 crossFingerVec3(const Vec3 left, const Vec3 right) noexcept
{
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

Vec3 rotateFingerVec3(const std::array<float, 4>& rotation, const Vec3 value) noexcept
{
    const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(rotation, {value.x, value.y, value.z});
    return {rotated[0], rotated[1], rotated[2]};
}

Vec3 negFingerVec3(const Vec3 value) noexcept
{
    return {-value.x, -value.y, -value.z};
}

int firstFingerChildIndex(const ProfileSkeletonJoints& joints, const int parent) noexcept
{
    for (std::size_t index = 0; index < joints.size(); ++index) {
        if (joints[index].parentIndex == parent) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

FingerBasis fingerBasisFor(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    const int child = firstFingerChildIndex(joints, joint);
    Vec3 y = parent >= 0
        ? subMappingVec3(joints[static_cast<std::size_t>(joint)].positionMeters, joints[static_cast<std::size_t>(parent)].positionMeters)
        : Vec3{0.0f, 1.0f, 0.0f};
    if (lengthMappingVec3(y) <= 0.0001f && child >= 0) {
        y = subMappingVec3(
            joints[static_cast<std::size_t>(child)].positionMeters,
            joints[static_cast<std::size_t>(joint)].positionMeters
        );
    }
    y = normalizeMappingVec3Or(y, Vec3{0.0f, 1.0f, 0.0f});
    Vec3 x = joints[static_cast<std::size_t>(joint)].sideHint;
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    x = normalizeMappingVec3Or(x, Vec3{1.0f, 0.0f, 0.0f});
    return {x, y, normalizeMappingVec3Or(crossFingerVec3(x, y), Vec3{0.0f, 0.0f, 1.0f})};
}

ProfileSkeletonHandSide handSideForJoint(const int joint) noexcept
{
    return joint >= kProfileJointRightHandThumb1 ? ProfileSkeletonHandSide::Right : ProfileSkeletonHandSide::Left;
}

FingerBasis handBasisForSide(const ProfileSkeletonJoints& joints, const ProfileSkeletonHandSide side) noexcept
{
    const int hand = profileHandRootJoint(side);
    const Vec3 wrist = joints[static_cast<std::size_t>(hand)].positionMeters;
    Vec3 y = subMappingVec3(
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Middle, 1))].positionMeters,
        wrist
    );
    y = normalizeMappingVec3Or(y, Vec3{side == ProfileSkeletonHandSide::Left ? 1.0f : -1.0f, 0.0f, 0.0f});
    Vec3 spread = subMappingVec3(
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Index, 1))].positionMeters,
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Pinky, 1))].positionMeters
    );
    spread = subMappingVec3(spread, scaleMappingVec3(y, dotMappingVec3(spread, y)));
    spread = normalizeMappingVec3Or(spread, Vec3{0.0f, 0.0f, 1.0f});
    const Vec3 fallback = side == ProfileSkeletonHandSide::Left
        ? Vec3{0.0f, 1.0f, 0.0f}
        : Vec3{0.0f, -1.0f, 0.0f};
    Vec3 x = normalizeMappingVec3Or(crossFingerVec3(spread, y), fallback);
    const Vec3 preferred =
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Middle, 1))].sideHint;
    if (lengthMappingVec3(preferred) > 0.0001f && dotMappingVec3(x, preferred) < 0.0f) {
        x = negFingerVec3(x);
    }
    return {x, y, normalizeMappingVec3Or(crossFingerVec3(x, y), spread)};
}

std::array<float, 4> quaternionFromBasisDelta(const FingerBasis& rest, const FingerBasis& pose) noexcept
{
    const float m00 = pose.x.x * rest.x.x + pose.y.x * rest.y.x + pose.z.x * rest.z.x;
    const float m01 = pose.x.x * rest.x.y + pose.y.x * rest.y.y + pose.z.x * rest.z.y;
    const float m02 = pose.x.x * rest.x.z + pose.y.x * rest.y.z + pose.z.x * rest.z.z;
    const float m10 = pose.x.y * rest.x.x + pose.y.y * rest.y.x + pose.z.y * rest.z.x;
    const float m11 = pose.x.y * rest.x.y + pose.y.y * rest.y.y + pose.z.y * rest.z.y;
    const float m12 = pose.x.y * rest.x.z + pose.y.y * rest.y.z + pose.z.y * rest.z.z;
    const float m20 = pose.x.z * rest.x.x + pose.y.z * rest.y.x + pose.z.z * rest.z.x;
    const float m21 = pose.x.z * rest.x.y + pose.y.z * rest.y.y + pose.z.z * rest.z.y;
    const float m22 = pose.x.z * rest.x.z + pose.y.z * rest.y.z + pose.z.z * rest.z.z;
    const float trace = m00 + m11 + m22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return ovtr::normalizeQuaternion({(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25f * s});
    }
    if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        return ovtr::normalizeQuaternion({0.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s});
    }
    if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        return ovtr::normalizeQuaternion({(m01 + m10) / s, 0.25f * s, (m12 + m21) / s, (m02 - m20) / s});
    }
    const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
    return ovtr::normalizeQuaternion({(m02 + m20) / s, (m12 + m21) / s, 0.25f * s, (m10 - m01) / s});
}

std::array<float, 4> shortestArcRotation(const Vec3 from, const Vec3 to) noexcept
{
    const Vec3 a = normalizeMappingVec3Or(from, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 b = normalizeMappingVec3Or(to, a);
    const float dot = dotMappingVec3(a, b);
    if (dot > 0.9999f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    if (dot < -0.9999f) {
        const Vec3 fallback = std::fabs(a.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
        const Vec3 axis = normalizeMappingVec3Or(crossFingerVec3(fallback, a), Vec3{1.0f, 0.0f, 0.0f});
        return {axis.x, axis.y, axis.z, 0.0f};
    }
    const Vec3 axis = crossFingerVec3(a, b);
    return ovtr::normalizeQuaternion({axis.x, axis.y, axis.z, 1.0f + dot});
}

std::array<float, 4> noRollFingerWorldRotation(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& target,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& world,
    const int parent,
    const int joint,
    const Vec3 worldDelta
) noexcept {
    const FingerBasis parentRestBasis = fingerBasisFor(rest, parent);
    const FingerBasis jointRestBasis = fingerBasisFor(rest, joint);
    const Vec3 y = normalizeMappingVec3Or(worldDelta, rotateFingerVec3(world[parent], jointRestBasis.y));
    const Vec3 parentSide = rotateFingerVec3(world[parent], parentRestBasis.x);
    const Vec3 targetSide = handBasisForSide(target, handSideForJoint(joint)).x;
    const Vec3 hint = lengthMappingVec3(targetSide) > 0.0001f ? targetSide : parentSide;
    Vec3 x = subMappingVec3(hint, scaleMappingVec3(y, dotMappingVec3(hint, y)));
    x = normalizeMappingVec3Or(x, rotateFingerVec3(world[parent], jointRestBasis.x));
    const Vec3 z = normalizeMappingVec3Or(crossFingerVec3(x, y), rotateFingerVec3(world[parent], jointRestBasis.z));
    return quaternionFromBasisDelta(jointRestBasis, {x, y, z});
}

void stabilizeHandRoot(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& target,
    SkeletonPose& pose,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& world,
    const ProfileSkeletonHandSide side
) {
    const int hand = profileHandRootJoint(side);
    const int parent = rest[static_cast<std::size_t>(hand)].parentIndex;
    if (parent < 0) {
        return;
    }
    const std::array<float, 4> desiredWorld =
        quaternionFromBasisDelta(handBasisForSide(rest, side), handBasisForSide(target, side));
    pose.bones[static_cast<std::size_t>(hand)].localRotation = ovtr::normalizeQuaternion(
        ovtr::multiplyQuaternion(ovtr::conjugateQuaternion(world[static_cast<std::size_t>(parent)]), desiredWorld)
    );
    world[static_cast<std::size_t>(hand)] = desiredWorld;
}

void stabilizeFingerJoint(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& target,
    SkeletonPose& pose,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& world,
    const ProfileSkeletonFinger finger,
    const int joint
) {
    const int parent = rest[static_cast<std::size_t>(joint)].parentIndex;
    if (parent < 0) {
        return;
    }
    const Vec3 worldDelta = subMappingVec3(
        target[static_cast<std::size_t>(joint)].positionMeters,
        target[static_cast<std::size_t>(parent)].positionMeters
    );
    const Vec3 localDelta = rotateFingerVec3(ovtr::conjugateQuaternion(world[static_cast<std::size_t>(parent)]), worldDelta);
    const Vec3 restDelta = subMappingVec3(
        rest[static_cast<std::size_t>(joint)].positionMeters,
        rest[static_cast<std::size_t>(parent)].positionMeters
    );
    pose.bones[static_cast<std::size_t>(joint)].localTranslationMeters = localDelta;
    if (finger == ProfileSkeletonFinger::Thumb) {
        pose.bones[static_cast<std::size_t>(joint)].localRotation = shortestArcRotation(restDelta, localDelta);
        world[static_cast<std::size_t>(joint)] = ovtr::multiplyQuaternion(
            world[static_cast<std::size_t>(parent)],
            pose.bones[static_cast<std::size_t>(joint)].localRotation
        );
    } else {
        world[static_cast<std::size_t>(joint)] =
            noRollFingerWorldRotation(rest, target, world, parent, joint, worldDelta);
        pose.bones[static_cast<std::size_t>(joint)].localRotation = ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
            ovtr::conjugateQuaternion(world[static_cast<std::size_t>(parent)]),
            world[static_cast<std::size_t>(joint)]
        ));
    }
    pose.bones[static_cast<std::size_t>(joint)].valid = true;
}

} // namespace

void stabilizeSkeletonFingerRolls(const ProfileSkeletonJoints& rest, SkeletonPose& pose)
{
    const ProfileSkeletonJoints target = computeSkeletonPoseWorldJoints(rest, pose);
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world =
        computeSkeletonPoseWorldRotations(rest, pose);
    for (const ProfileSkeletonHandSide side : {ProfileSkeletonHandSide::Left, ProfileSkeletonHandSide::Right}) {
        stabilizeHandRoot(rest, target, pose, world, side);
        for (const ProfileSkeletonFinger finger : {
            ProfileSkeletonFinger::Thumb,
            ProfileSkeletonFinger::Index,
            ProfileSkeletonFinger::Middle,
            ProfileSkeletonFinger::Ring,
            ProfileSkeletonFinger::Pinky,
        }) {
            for (int segment = 1; segment <= kProfileFingerJointSegments; ++segment) {
                stabilizeFingerJoint(rest, target, pose, world, finger, profileFingerJoint(side, finger, segment));
            }
        }
    }
}

} // namespace ovtr::win32
