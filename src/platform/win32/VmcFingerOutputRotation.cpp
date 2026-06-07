#include "platform/win32/VmcFingerOutputRotation.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonGltfPoseBasis.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/VmcLegRotationContinuity.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

std::array<float, 4> sameQuaternionHemisphere(
    std::array<float, 4> q,
    const std::array<float, 4>& reference
) noexcept {
    if (q[0] * reference[0] + q[1] * reference[1] + q[2] * reference[2] + q[3] * reference[3] < 0.0f) {
        for (float& value : q) {
            value = -value;
        }
    }
    return q;
}

int firstChildIndex(const ProfileSkeletonJoints& joints, const int parent) noexcept
{
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        if (joints[static_cast<std::size_t>(joint)].parentIndex == parent) {
            return joint;
        }
    }
    return -1;
}

Vec3 fingerDirectionFor(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const int joint,
    const SkeletonGltfBasis& fallback
) noexcept {
    const int child = firstChildIndex(rest, joint);
    if (child >= 0) {
        return normalizeMappingVec3Or(
            subMappingVec3(
                poseJoints[static_cast<std::size_t>(child)].positionMeters,
                poseJoints[static_cast<std::size_t>(joint)].positionMeters
            ),
            fallback.y
        );
    }
    return fallback.y;
}

Vec3 rotateVec3ByQuaternion(const std::array<float, 4>& q, const Vec3 value) noexcept
{
    const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(q, {value.x, value.y, value.z});
    return Vec3{rotated[0], rotated[1], rotated[2]};
}

Vec3 parentLocalFingerDirection(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const int joint,
    const std::array<float, 4>& parentWorld,
    const Vec3 fallback
) noexcept {
    return normalizeMappingVec3Or(
        rotateVec3ByQuaternion(
            ovtr::conjugateQuaternion(parentWorld),
            fingerDirectionFor(rest, poseJoints, joint, SkeletonGltfBasis{{}, fallback, {}})
        ),
        fallback
    );
}

SkeletonGltfBasis fingerLocalBasisFor(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& joints,
    const ProfileSkeletonHandSide side,
    const int joint
) noexcept {
    const SkeletonGltfBasis restBasis = skeletonGltfExportBasisFor(rest, joint);
    const Vec3 y = fingerDirectionFor(rest, joints, joint, restBasis);
    const int hand = profileHandRootJoint(side);
    const SkeletonGltfBasis handBasis = skeletonGltfExportBasisFor(joints, hand);
    return skeletonGltfBasisFromYAndXHint(y, handBasis.x, restBasis);
}

void applyFingerJointRotation(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const ProfileSkeletonHandSide side,
    const int joint,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    VmcLegRotationContinuity* continuity
) noexcept {
    const std::size_t index = static_cast<std::size_t>(joint);
    const int parent = rest[index].parentIndex;
    const std::array<float, 4> parentWorld = parent >= 0
        ? worldRotations[static_cast<std::size_t>(parent)]
        : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
    const Vec3 restLocalY = fingerDirectionFor(
        rest,
        rest,
        joint,
        fingerLocalBasisFor(rest, rest, side, joint)
    );
    const Vec3 poseLocalY = parentLocalFingerDirection(rest, poseJoints, joint, parentWorld, restLocalY);
    const std::array<float, 4> localRotation = skeletonGltfSwingBetween(restLocalY, poseLocalY);
    std::array<float, 4> worldRotation = ovtr::normalizeQuaternion(
        ovtr::multiplyQuaternion(parentWorld, localRotation)
    );
    if (continuity && continuity->valid[index]) {
        worldRotation = sameQuaternionHemisphere(worldRotation, continuity->previousWorld[index]);
    }
    localRotations[index] = parent >= 0
        ? ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(ovtr::conjugateQuaternion(parentWorld), worldRotation))
        : worldRotation;
    worldRotations[index] = worldRotation;
    if (continuity) {
        continuity->previousWorld[index] = worldRotation;
        continuity->valid[index] = true;
    }
}

void applyFingerRotationsForSide(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const ProfileSkeletonHandSide side,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    VmcLegRotationContinuity* continuity
) noexcept {
    for (int finger = 0; finger < 5; ++finger) {
        const auto fingerId = static_cast<ProfileSkeletonFinger>(finger);
        for (int segment = 1; segment <= 3; ++segment) {
            applyFingerJointRotation(
                rest,
                poseJoints,
                side,
                profileFingerJoint(side, fingerId, segment),
                localRotations,
                worldRotations,
                continuity
            );
        }
    }
}

} // namespace

SkeletonGltfBasis vmcFingerOutputPoseBasisForJoint(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    const ProfileSkeletonHandSide side,
    const int joint
) noexcept {
    (void)worldRotations;
    return fingerLocalBasisFor(rest, poseJoints, side, joint);
}

void applyVmcFingerOutputRotations(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    std::array<std::array<float, 4>, kProfileSkeletonJointCount>& worldRotations,
    VmcLegRotationContinuity* continuity
) noexcept {
    applyFingerRotationsForSide(rest, poseJoints, ProfileSkeletonHandSide::Left, localRotations, worldRotations, continuity);
    applyFingerRotationsForSide(rest, poseJoints, ProfileSkeletonHandSide::Right, localRotations, worldRotations, continuity);
}

} // namespace ovtr::win32
