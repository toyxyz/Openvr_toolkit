#include "platform/win32/SkeletonGltfPoseBasis.h"

#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

int primaryChildIndex(const ProfileSkeletonJoints& joints, const int parent) noexcept
{
    if (parent == kProfileJointLeftHand) {
        return kProfileJointLeftHandMiddle1;
    }
    if (parent == kProfileJointRightHand) {
        return kProfileJointRightHandMiddle1;
    }
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (joints[static_cast<std::size_t>(joint)].parentIndex == parent) {
            return joint;
        }
    }
    return -1;
}

bool isUpLegJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftUpLeg || joint == kProfileJointRightUpLeg;
}

bool isHandJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftHand || joint == kProfileJointRightHand;
}

ProfileSkeletonHandSide handSideForFingerJoint(const int joint) noexcept
{
    return joint >= kProfileJointRightHandThumb1 ? ProfileSkeletonHandSide::Right : ProfileSkeletonHandSide::Left;
}

Vec3 fallbackSideFor(const Vec3 y) noexcept
{
    return std::fabs(y.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
}

Vec3 projectedBasisX(const Vec3 y, const Vec3 hint, const Vec3 fallback) noexcept
{
    Vec3 x = subMappingVec3(hint, scaleMappingVec3(y, dotMappingVec3(hint, y)));
    return normalizeMappingVec3Or(x, fallback);
}

float quaternionAbsDot(const std::array<float, 4>& a, const std::array<float, 4>& b) noexcept
{
    return std::fabs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
}

Vec3 handRollHintFor(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    if (parent >= 0 && lengthMappingVec3(joints[static_cast<std::size_t>(parent)].sideHint) > 0.0001f) {
        return joints[static_cast<std::size_t>(parent)].sideHint;
    }
    if (parent >= 0) {
        const Vec3 parentY = subMappingVec3(
            joints[static_cast<std::size_t>(joint)].positionMeters,
            joints[static_cast<std::size_t>(parent)].positionMeters
        );
        if (lengthMappingVec3(parentY) > 0.0001f) {
            const Vec3 axisY = normalizeMappingVec3Or(parentY, Vec3{0.0f, 1.0f, 0.0f});
            const Vec3 fallback = normalizeMappingVec3Or(cross(axisY, fallbackSideFor(axisY)), Vec3{1.0f, 0.0f, 0.0f});
            return projectedBasisX(axisY, joints[static_cast<std::size_t>(parent)].sideHint, fallback);
        }
    }
    return joint == kProfileJointRightHand ? Vec3{0.0f, 0.0f, -1.0f} : Vec3{0.0f, 0.0f, 1.0f};
}

Vec3 fingerPalmHintFor(const ProfileSkeletonJoints& joints, const int joint, const Vec3 y) noexcept
{
    const ProfileSkeletonHandSide side = handSideForFingerJoint(joint);
    const int hand = profileHandRootJoint(side);
    Vec3 forward = subMappingVec3(
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Middle, 1))].positionMeters,
        joints[static_cast<std::size_t>(hand)].positionMeters
    );
    forward = normalizeMappingVec3Or(
        forward,
        Vec3{side == ProfileSkeletonHandSide::Left ? 1.0f : -1.0f, 0.0f, 0.0f}
    );
    Vec3 spread = subMappingVec3(
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Index, 1))].positionMeters,
        joints[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Pinky, 1))].positionMeters
    );
    spread = subMappingVec3(spread, scaleMappingVec3(forward, dotMappingVec3(spread, forward)));
    spread = normalizeMappingVec3Or(spread, Vec3{0.0f, 0.0f, 1.0f});
    const Vec3 palm = normalizeMappingVec3Or(
        cross(spread, forward),
        side == ProfileSkeletonHandSide::Left ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, -1.0f, 0.0f}
    );
    Vec3 projectedPalm = subMappingVec3(palm, scaleMappingVec3(y, dotMappingVec3(palm, y)));
    if (lengthMappingVec3(projectedPalm) > 0.0001f) {
        return normalizeMappingVec3Or(projectedPalm, palm);
    }
    Vec3 tangent = cross(spread, y);
    if (dotMappingVec3(tangent, palm) < 0.0f) {
        tangent = scaleMappingVec3(tangent, -1.0f);
    }
    return normalizeMappingVec3Or(tangent, fallbackSideFor(y));
}

} // namespace

std::array<Vec3, kProfileSkeletonJointCount> exportSkeletonGltfWorldPositions(const ProfileSkeletonJoints& joints)
{
    std::array<Vec3, kProfileSkeletonJointCount> positions{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        positions[static_cast<std::size_t>(joint)] =
            joints[static_cast<std::size_t>(joint)].positionMeters;
    }
    positions[kProfileJointSpine] = joints[kProfileJointHips].positionMeters;
    positions[kProfileJointSpine1] = joints[kProfileJointSpine].positionMeters;
    positions[kProfileJointSpine2] = joints[kProfileJointSpine1].positionMeters;
    positions[kProfileJointNeck] = joints[kProfileJointSpine2].positionMeters;
    positions[kProfileJointHead] = joints[kProfileJointNeck].positionMeters;
    positions[kProfileJointLeftShoulder] = positions[kProfileJointNeck];
    positions[kProfileJointLeftArm] = joints[kProfileJointLeftShoulder].positionMeters;
    positions[kProfileJointLeftForeArm] = joints[kProfileJointLeftArm].positionMeters;
    positions[kProfileJointLeftHand] = joints[kProfileJointLeftForeArm].positionMeters;
    positions[kProfileJointRightShoulder] = positions[kProfileJointNeck];
    positions[kProfileJointRightArm] = joints[kProfileJointRightShoulder].positionMeters;
    positions[kProfileJointRightForeArm] = joints[kProfileJointRightArm].positionMeters;
    positions[kProfileJointRightHand] = joints[kProfileJointRightForeArm].positionMeters;
    return positions;
}

ProfileSkeletonJoints exportSkeletonGltfJoints(const ProfileSkeletonJoints& joints)
{
    ProfileSkeletonJoints out = joints;
    const auto positions = exportSkeletonGltfWorldPositions(joints);
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        out[static_cast<std::size_t>(joint)].positionMeters = positions[static_cast<std::size_t>(joint)];
    }
    return out;
}

SkeletonGltfBasis skeletonGltfExportBasisFor(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int child = primaryChildIndex(joints, joint);
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    Vec3 y = child >= 0
        ? subMappingVec3(joints[static_cast<std::size_t>(child)].positionMeters, joints[static_cast<std::size_t>(joint)].positionMeters)
        : Vec3{};
    if (lengthMappingVec3(y) <= 0.0001f && parent >= 0) {
        y = subMappingVec3(joints[static_cast<std::size_t>(joint)].positionMeters, joints[static_cast<std::size_t>(parent)].positionMeters);
    }
    y = normalizeMappingVec3Or(y, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 fallback = normalizeMappingVec3Or(cross(y, fallbackSideFor(y)), Vec3{1.0f, 0.0f, 0.0f});
    Vec3 hint = joints[static_cast<std::size_t>(joint)].sideHint;
    if (isUpLegJoint(joint)) {
        hint = Vec3{0.0f, 0.0f, 1.0f};
    }
    if (isHandJoint(joint)) {
        hint = handRollHintFor(joints, joint);
    }
    if (isProfileFingerJoint(joint)) {
        hint = fingerPalmHintFor(joints, joint, y);
    }
    const Vec3 x = projectedBasisX(y, hint, fallback);
    return SkeletonGltfBasis{x, y, normalizeMappingVec3Or(cross(x, y), Vec3{0.0f, 0.0f, 1.0f})};
}

SkeletonGltfBasis alignSkeletonGltfBasisRoll(const SkeletonGltfBasis basis, const SkeletonGltfBasis rest) noexcept
{
    if (dotMappingVec3(basis.x, rest.x) >= 0.0f) {
        return basis;
    }
    return SkeletonGltfBasis{scaleMappingVec3(basis.x, -1.0f), basis.y, scaleMappingVec3(basis.z, -1.0f)};
}

SkeletonGltfBasis skeletonGltfBasisFromYAndXHint(
    const Vec3 y,
    const Vec3 xHint,
    const SkeletonGltfBasis fallback
) noexcept {
    const Vec3 axisY = normalizeMappingVec3Or(y, fallback.y);
    const Vec3 axisX = projectedBasisX(axisY, xHint, fallback.x);
    return SkeletonGltfBasis{axisX, axisY, normalizeMappingVec3Or(cross(axisX, axisY), fallback.z)};
}

std::array<float, 4> skeletonGltfQuaternionFromBasis(const SkeletonGltfBasis& b) noexcept
{
    const float trace = b.x.x + b.y.y + b.z.z;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return ovtr::normalizeQuaternion({(b.y.z - b.z.y) / s, (b.z.x - b.x.z) / s, (b.x.y - b.y.x) / s, 0.25f * s});
    }
    if (b.x.x > b.y.y && b.x.x > b.z.z) {
        const float s = std::sqrt(1.0f + b.x.x - b.y.y - b.z.z) * 2.0f;
        return ovtr::normalizeQuaternion({0.25f * s, (b.y.x + b.x.y) / s, (b.z.x + b.x.z) / s, (b.y.z - b.z.y) / s});
    }
    if (b.y.y > b.z.z) {
        const float s = std::sqrt(1.0f + b.y.y - b.x.x - b.z.z) * 2.0f;
        return ovtr::normalizeQuaternion({(b.y.x + b.x.y) / s, 0.25f * s, (b.z.y + b.y.z) / s, (b.z.x - b.x.z) / s});
    }
    const float s = std::sqrt(1.0f + b.z.z - b.x.x - b.y.y) * 2.0f;
    return ovtr::normalizeQuaternion({(b.z.x + b.x.z) / s, (b.z.y + b.y.z) / s, 0.25f * s, (b.x.y - b.y.x) / s});
}

std::array<float, 4> closestSkeletonGltfRoll(
    const SkeletonGltfBasis basis,
    const std::array<float, 4>& previous
) noexcept {
    const std::array<float, 4> q = skeletonGltfQuaternionFromBasis(basis);
    const std::array<float, 4> flipped = skeletonGltfQuaternionFromBasis(
        SkeletonGltfBasis{scaleMappingVec3(basis.x, -1.0f), basis.y, scaleMappingVec3(basis.z, -1.0f)}
    );
    return quaternionAbsDot(flipped, previous) > quaternionAbsDot(q, previous) ? flipped : q;
}

std::array<float, 4> skeletonGltfSwingBetween(const Vec3 from, const Vec3 to) noexcept
{
    const Vec3 a = normalizeMappingVec3Or(from, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 b = normalizeMappingVec3Or(to, Vec3{0.0f, 1.0f, 0.0f});
    const float d = dotMappingVec3(a, b);
    if (d < -0.9999f) {
        const Vec3 axis = normalizeMappingVec3Or(cross(a, fallbackSideFor(a)), Vec3{1.0f, 0.0f, 0.0f});
        return {axis.x, axis.y, axis.z, 0.0f};
    }
    const Vec3 axis = cross(a, b);
    return ovtr::normalizeQuaternion({axis.x, axis.y, axis.z, 1.0f + d});
}

Vec3 skeletonGltfPrimaryBoneDirection(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int child = primaryChildIndex(joints, joint);
    return child >= 0
        ? subMappingVec3(joints[static_cast<std::size_t>(child)].positionMeters, joints[static_cast<std::size_t>(joint)].positionMeters)
        : Vec3{0.0f, 1.0f, 0.0f};
}

} // namespace ovtr::win32
