#include "platform/win32/VmcArmRotationContinuity.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr int kVmcArmRotationJoints[] = {
    kProfileJointLeftArm,
    kProfileJointLeftForeArm,
    kProfileJointRightArm,
    kProfileJointRightForeArm,
};

struct Basis {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

float quaternionDot(const std::array<float, 4>& a, const std::array<float, 4>& b) noexcept
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

float quaternionAbsDot(const std::array<float, 4>& a, const std::array<float, 4>& b) noexcept
{
    return std::fabs(quaternionDot(a, b));
}

std::array<float, 4> sameQuaternionHemisphere(
    std::array<float, 4> q,
    const std::array<float, 4>& reference
) noexcept {
    if (quaternionDot(q, reference) < 0.0f) {
        for (float& value : q) {
            value = -value;
        }
    }
    return q;
}

bool isForeArmJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftForeArm || joint == kProfileJointRightForeArm;
}

int handJointForForeArm(const int joint) noexcept
{
    return joint == kProfileJointLeftForeArm ? kProfileJointLeftHand : kProfileJointRightHand;
}

Vec3 crossVec(const Vec3 a, const Vec3 b) noexcept
{
    return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 rotateVec3ByQuaternion(const std::array<float, 4>& q, const Vec3 value) noexcept
{
    const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(q, {value.x, value.y, value.z});
    return Vec3{rotated[0], rotated[1], rotated[2]};
}

Vec3 fallbackSideFor(const Vec3 y) noexcept
{
    return std::fabs(y.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
}

Vec3 jointPrimaryDirection(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    return parent >= 0
        ? normalizeMappingVec3Or(subMappingVec3(
            joints[static_cast<std::size_t>(joint)].positionMeters,
            joints[static_cast<std::size_t>(parent)].positionMeters
        ), Vec3{0.0f, 1.0f, 0.0f})
        : Vec3{0.0f, 1.0f, 0.0f};
}

Basis jointBasis(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const Vec3 y = jointPrimaryDirection(joints, joint);
    Vec3 x = joints[static_cast<std::size_t>(joint)].sideHint;
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    x = normalizeMappingVec3Or(x, normalizeMappingVec3Or(crossVec(y, fallbackSideFor(y)), Vec3{1.0f, 0.0f, 0.0f}));
    return Basis{x, y, normalizeMappingVec3Or(crossVec(x, y), Vec3{0.0f, 0.0f, 1.0f})};
}

std::array<float, 4> quaternionFromBasisDelta(const Basis& rest, const Basis& pose) noexcept
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

std::array<float, 4> foreArmHingeWorldRotation(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const int joint,
    const std::array<float, 4>& parentWorld,
    const VmcLegRotationContinuity& continuity
) noexcept {
    const Basis restBasis = jointBasis(rest, joint);
    const Vec3 y = jointPrimaryDirection(poseJoints, joint);
    const Vec3 parentX = rotateVec3ByQuaternion(parentWorld, restBasis.x);
    Vec3 x = subMappingVec3(parentX, scaleMappingVec3(y, dotMappingVec3(parentX, y)));
    x = normalizeMappingVec3Or(x, restBasis.x);
    const Basis poseBasis{x, y, normalizeMappingVec3Or(crossVec(x, y), restBasis.z)};
    const Basis flippedBasis{scaleMappingVec3(x, -1.0f), y, scaleMappingVec3(poseBasis.z, -1.0f)};
    const std::array<float, 4> candidate = quaternionFromBasisDelta(restBasis, poseBasis);
    const std::array<float, 4> flipped = quaternionFromBasisDelta(restBasis, flippedBasis);
    const std::size_t index = static_cast<std::size_t>(joint);
    std::array<float, 4> reference = continuity.previousWorld[index];
    if (!continuity.valid[index]) {
        const int parent = rest[static_cast<std::size_t>(joint)].parentIndex;
        const std::array<float, 4> restLocal = parent >= 0
            ? quaternionFromBasisDelta(jointBasis(rest, parent), restBasis)
            : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
        reference = parent >= 0
            ? ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(parentWorld, restLocal))
            : restLocal;
    }
    return quaternionAbsDot(flipped, reference) > quaternionAbsDot(candidate, reference)
        ? flipped
        : candidate;
}

std::array<float, 4> stableArmWorldRotation(
    const int joint,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const std::array<float, 4>& rawWorld,
    const std::array<float, 4>& parentWorld,
    const VmcLegRotationContinuity& continuity
) noexcept {
    const std::size_t index = static_cast<std::size_t>(joint);
    std::array<float, 4> candidate = isForeArmJoint(joint)
        ? foreArmHingeWorldRotation(rest, poseJoints, joint, parentWorld, continuity)
        : rawWorld;
    if (!continuity.valid[index]) {
        return candidate;
    }
    return sameQuaternionHemisphere(candidate, continuity.previousWorld[index]);
}

} // namespace

void applyVmcArmRotationsWithContinuity(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    VmcLegRotationContinuity& continuity
) noexcept {
    const auto rawWorldRotations = worldRotations;
    for (const int joint : kVmcArmRotationJoints) {
        const std::size_t index = static_cast<std::size_t>(joint);
        const int parent = rest[index].parentIndex;
        const std::array<float, 4> parentWorld = parent >= 0
            ? worldRotations[static_cast<std::size_t>(parent)]
            : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
        const std::array<float, 4> worldRotation =
            stableArmWorldRotation(joint, rest, poseJoints, rawWorldRotations[index], parentWorld, continuity);

        localRotations[index] = parent >= 0
            ? ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
                ovtr::conjugateQuaternion(parentWorld),
                worldRotation
            ))
            : worldRotation;
        worldRotations[index] = worldRotation;
        continuity.previousWorld[index] = worldRotation;
        continuity.valid[index] = true;

        if (isForeArmJoint(joint)) {
            const int hand = handJointForForeArm(joint);
            const auto handIndex = static_cast<std::size_t>(hand);
            localRotations[handIndex] = ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
                ovtr::conjugateQuaternion(worldRotation),
                rawWorldRotations[handIndex]
            ));
            worldRotations[handIndex] = rawWorldRotations[handIndex];
        }
    }
}

} // namespace ovtr::win32
