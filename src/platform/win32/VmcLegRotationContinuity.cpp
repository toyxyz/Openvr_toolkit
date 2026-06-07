#include "platform/win32/VmcLegRotationContinuity.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/VmcArmRotationContinuity.h"
#include "platform/win32/VmcFingerOutputRotation.h"
#include "platform/win32/SkeletonGltfPoseBasis.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr int kVmcLegRotationJoints[] = {
    kProfileJointLeftUpLeg,
    kProfileJointLeftLeg,
    kProfileJointLeftFoot,
    kProfileJointLeftToeBase,
    kProfileJointRightUpLeg,
    kProfileJointRightLeg,
    kProfileJointRightFoot,
    kProfileJointRightToeBase,
};

bool vmcLegRollNeedsFlip(const int joint) noexcept
{
    (void)joint;
    return false;
}

bool isUpperLegJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftUpLeg || joint == kProfileJointRightUpLeg;
}

bool preservesLegRollJoint(const int joint) noexcept
{
    return isUpperLegJoint(joint) || joint == kProfileJointLeftFoot || joint == kProfileJointRightFoot;
}

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

SkeletonGltfBasis flippedBasis(const SkeletonGltfBasis basis) noexcept
{
    return SkeletonGltfBasis{
        scaleMappingVec3(basis.x, -1.0f),
        basis.y,
        scaleMappingVec3(basis.z, -1.0f)
    };
}

Vec3 rotateVec3ByQuaternion(const std::array<float, 4>& q, const Vec3 value) noexcept
{
    const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(q, {value.x, value.y, value.z});
    return Vec3{rotated[0], rotated[1], rotated[2]};
}

bool rollHintProjectsOntoPlane(const Vec3 y, const Vec3 hint) noexcept
{
    const Vec3 projected = subMappingVec3(hint, scaleMappingVec3(y, dotMappingVec3(hint, y)));
    return lengthMappingVec3(projected) > 0.0001f;
}

Vec3 vmcLegDirectionFor(
    const ProfileSkeletonJoints& joints,
    const int joint,
    const Vec3 fallback
) noexcept {
    if (joint == kProfileJointLeftToeBase || joint == kProfileJointRightToeBase) {
        const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
        if (parent >= 0) {
            return normalizeMappingVec3Or(
                subMappingVec3(
                    joints[static_cast<std::size_t>(joint)].positionMeters,
                    joints[static_cast<std::size_t>(parent)].positionMeters
                ),
                fallback
            );
        }
    }
    return normalizeMappingVec3Or(skeletonGltfPrimaryBoneDirection(joints, joint), fallback);
}

SkeletonGltfBasis vmcLegBasisFor(
    const ProfileSkeletonJoints& joints,
    const int joint,
    const SkeletonGltfBasis& fallback
) noexcept {
    const Vec3 y = vmcLegDirectionFor(joints, joint, fallback.y);
    Vec3 xHint = joints[static_cast<std::size_t>(joint)].sideHint;
    if (lengthMappingVec3(xHint) <= 0.0001f) {
        xHint = fallback.x;
    }
    SkeletonGltfBasis basis = skeletonGltfBasisFromYAndXHint(y, xHint, fallback);
    return vmcLegRollNeedsFlip(joint) ? flippedBasis(basis) : basis;
}

bool upperLegParentBasis(
    const SkeletonGltfBasis& restBasis,
    const Vec3 y,
    const int joint,
    const std::array<float, 4>& parentWorld,
    SkeletonGltfBasis& out
) noexcept {
    if (!isUpperLegJoint(joint)) {
        return false;
    }
    const Vec3 parentX = rotateVec3ByQuaternion(parentWorld, restBasis.x);
    if (!rollHintProjectsOntoPlane(y, parentX)) {
        return false;
    }
    out = skeletonGltfBasisFromYAndXHint(y, parentX, restBasis);
    if (vmcLegRollNeedsFlip(joint)) {
        out = flippedBasis(out);
    }
    return true;
}

std::array<float, 4> worldRotationFromBasis(
    const SkeletonGltfBasis& restBasis,
    const SkeletonGltfBasis& poseBasis
) noexcept {
    return ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
        skeletonGltfQuaternionFromBasis(poseBasis),
        ovtr::conjugateQuaternion(skeletonGltfQuaternionFromBasis(restBasis))
    ));
}

std::array<float, 4> restReferenceWorldRotation(
    const ProfileSkeletonJoints& rest,
    const int joint,
    const std::array<float, 4>& parentWorld
) noexcept {
    const int parent = rest[static_cast<std::size_t>(joint)].parentIndex;
    if (parent < 0) {
        return std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
    }
    const SkeletonGltfBasis restBasis = skeletonGltfExportBasisFor(rest, joint);
    const std::array<float, 4> restLocal = worldRotationFromBasis(
        skeletonGltfExportBasisFor(rest, parent),
        restBasis
    );
    return ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(parentWorld, restLocal));
}

std::array<float, 4> swingOnlyWorldRotation(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const int joint,
    const std::array<float, 4>& parentWorld
) noexcept {
    const SkeletonGltfBasis restBasis = skeletonGltfExportBasisFor(rest, joint);
    const Vec3 restLocalY = vmcLegDirectionFor(rest, joint, restBasis.y);
    const Vec3 poseLocalY = normalizeMappingVec3Or(
        rotateVec3ByQuaternion(
            ovtr::conjugateQuaternion(parentWorld),
            vmcLegDirectionFor(poseJoints, joint, restBasis.y)
        ),
        restLocalY
    );
    return ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
        parentWorld,
        skeletonGltfSwingBetween(restLocalY, poseLocalY)
    ));
}

std::array<float, 4> closestWorldRotation(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const int joint,
    const std::array<float, 4>& parentWorld,
    const VmcLegRotationContinuity& continuity
) noexcept {
    const SkeletonGltfBasis restBasis = skeletonGltfExportBasisFor(rest, joint);
    const Vec3 y = vmcLegDirectionFor(poseJoints, joint, restBasis.y);
    SkeletonGltfBasis poseBasis{};
    const bool parentAnchored = upperLegParentBasis(restBasis, y, joint, parentWorld, poseBasis);
    if (!parentAnchored) {
        poseBasis = vmcLegBasisFor(poseJoints, joint, restBasis);
    }
    std::array<float, 4> candidate = worldRotationFromBasis(restBasis, poseBasis);
    const std::size_t index = static_cast<std::size_t>(joint);
    const std::array<float, 4> reference = continuity.valid[index]
        ? continuity.previousWorld[index]
        : restReferenceWorldRotation(rest, joint, parentWorld);
    if (parentAnchored) {
        return sameQuaternionHemisphere(candidate, reference);
    }
    const SkeletonGltfBasis alternateBasis = flippedBasis(poseBasis);
    const std::array<float, 4> alternate = worldRotationFromBasis(restBasis, alternateBasis);
    if (quaternionAbsDot(alternate, reference) > quaternionAbsDot(candidate, reference)) {
        candidate = alternate;
    }
    return sameQuaternionHemisphere(candidate, reference);
}

} // namespace

void resetVmcLegRotationContinuity(VmcLegRotationContinuity& continuity) noexcept
{
    continuity.previousWorld = {};
    continuity.valid = {};
}

std::array<std::array<float, 4>, kProfileSkeletonJointCount> makeVmcLocalRotationsWithContinuity(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& poseJoints,
    const SkeletonPose& pose,
    VmcLegRotationContinuity& continuity
) {
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> local{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        local[static_cast<std::size_t>(joint)] = pose.bones[static_cast<std::size_t>(joint)].localRotation;
    }

    auto world = computeSkeletonPoseWorldRotations(rest, pose);
    const auto rawWorld = world;
    for (const int joint : kVmcLegRotationJoints) {
        const std::size_t index = static_cast<std::size_t>(joint);
        const int parent = rest[static_cast<std::size_t>(joint)].parentIndex;
        const std::array<float, 4> parentWorld =
            parent >= 0 ? world[static_cast<std::size_t>(parent)] : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
        const std::array<float, 4> worldRotation = preservesLegRollJoint(joint)
            ? closestWorldRotation(rest, poseJoints, joint, parentWorld, continuity)
            : sameQuaternionHemisphere(
                swingOnlyWorldRotation(rest, poseJoints, joint, parentWorld),
                continuity.valid[index] ? continuity.previousWorld[index] : rawWorld[index]
            );
        local[index] = parent >= 0
            ? ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
                ovtr::conjugateQuaternion(parentWorld),
                worldRotation
            ))
            : worldRotation;
        world[index] = worldRotation;
        continuity.previousWorld[index] = worldRotation;
        continuity.valid[index] = true;
    }
    applyVmcArmRotationsWithContinuity(rest, poseJoints, local, world, continuity);
    applyVmcFingerOutputRotations(rest, poseJoints, local, world, &continuity);
    return local;
}

} // namespace ovtr::win32
