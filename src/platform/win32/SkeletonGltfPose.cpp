#include "platform/win32/SkeletonGltfPose.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonFingerRoll.h"
#include "platform/win32/SkeletonGltfHierarchy.h"
#include "platform/win32/SkeletonGltfPoseBasis.h"

#include <array>
#include <cstddef>
#include <vector>

namespace ovtr::win32 {
namespace {

bool isLegAimJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftUpLeg || joint == kProfileJointRightUpLeg ||
        joint == kProfileJointLeftLeg || joint == kProfileJointRightLeg;
}

bool isArmAimJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftArm || joint == kProfileJointRightArm ||
        joint == kProfileJointLeftForeArm || joint == kProfileJointRightForeArm;
}

bool isFootJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftFoot || joint == kProfileJointRightFoot;
}

bool isHandJoint(const int joint) noexcept
{
    return joint == kProfileJointLeftHand || joint == kProfileJointRightHand;
}

Vec3 crossVec(const Vec3 a, const Vec3 b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
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

Vec3 fingerBoneDirection(const ProfileSkeletonJoints& joints, const int joint) noexcept
{
    const int child = firstChildIndex(joints, joint);
    if (child >= 0) {
        return subMappingVec3(joints[static_cast<std::size_t>(child)].positionMeters,
            joints[static_cast<std::size_t>(joint)].positionMeters);
    }
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    return parent >= 0
        ? subMappingVec3(joints[static_cast<std::size_t>(joint)].positionMeters,
            joints[static_cast<std::size_t>(parent)].positionMeters)
        : Vec3{0.0f, 1.0f, 0.0f};
}

Vec3 rotatedAxis(const std::array<float, 4>& rotation, const Vec3 axis) noexcept
{
    const std::array<float, 3> out = ovtr::rotatePositionByQuaternion(
        rotation,
        {axis.x, axis.y, axis.z}
    );
    return Vec3{out[0], out[1], out[2]};
}

SkeletonGltfBasis basisFromYAndZHint(const Vec3 y, const Vec3 zHint, const SkeletonGltfBasis fallback) noexcept
{
    const Vec3 axisY = normalizeMappingVec3Or(y, fallback.y);
    Vec3 axisZ = subMappingVec3(zHint, scaleMappingVec3(axisY, dotMappingVec3(zHint, axisY)));
    axisZ = normalizeMappingVec3Or(axisZ, fallback.z);
    const Vec3 axisX = normalizeMappingVec3Or(crossVec(axisY, axisZ), fallback.x);
    return SkeletonGltfBasis{axisX, axisY, normalizeMappingVec3Or(crossVec(axisX, axisY), axisZ)};
}

std::array<float, 4> parentRelativeRestRotation(
    const ProfileSkeletonJoints& rest,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& restWorld,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& world,
    const int joint
) noexcept {
    const int parent = skeletonGltfParentIndex(rest, joint);
    const auto restLocal = ovtr::multiplyQuaternion(
        ovtr::conjugateQuaternion(restWorld[static_cast<std::size_t>(parent)]),
        restWorld[static_cast<std::size_t>(joint)]
    );
    return ovtr::normalizeQuaternion(
        ovtr::multiplyQuaternion(world[static_cast<std::size_t>(parent)], restLocal)
    );
}

Vec3 toLocalDelta(const std::array<float, 4>& parentRotation, const Vec3 delta) noexcept
{
    const std::array<float, 3> local = ovtr::rotatePositionByQuaternion(
        ovtr::conjugateQuaternion(parentRotation),
        {delta.x, delta.y, delta.z}
    );
    return Vec3{local[0], local[1], local[2]};
}

std::array<std::array<float, 4>, kProfileSkeletonJointCount> exportWorldRotations(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
) {
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        const auto& local = pose.bones[static_cast<std::size_t>(joint)].localRotation;
        world[static_cast<std::size_t>(joint)] = parent >= 0
            ? ovtr::multiplyQuaternion(world[static_cast<std::size_t>(parent)], local)
            : local;
    }
    return world;
}

std::array<std::array<float, 4>, kProfileSkeletonJointCount> restExportWorldRotations(const ProfileSkeletonJoints& rest) noexcept
{
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        world[static_cast<std::size_t>(joint)] =
            skeletonGltfQuaternionFromBasis(skeletonGltfExportBasisFor(rest, joint));
    }
    return world;
}

SkeletonPose makeSkeletonGltfExportPoseInternal(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose,
    const bool alignRollToRest,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>* previousWorld
)
{
    SkeletonPose sourcePose = pose;
    stabilizeSkeletonFingerRolls(rest, sourcePose);
    const ProfileSkeletonJoints worldJoints = computeSkeletonPoseWorldJoints(rest, sourcePose);
    const ProfileSkeletonJoints restExportJoints = exportSkeletonGltfJoints(rest);
    const ProfileSkeletonJoints posedExportJoints = exportSkeletonGltfJoints(worldJoints);
    const auto restPositions = exportSkeletonGltfWorldPositions(rest);
    const auto restWorldRotations = restExportWorldRotations(restExportJoints);
    const auto sourceSideAxes = computeSkeletonPoseWorldSideAxes(rest, sourcePose);
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> worldRotations{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const std::size_t index = static_cast<std::size_t>(joint);
        const bool animationPose = !alignRollToRest;
        if (animationPose && isHandJoint(joint)) {
            const auto parentRelativeRest =
                parentRelativeRestRotation(rest, restWorldRotations, worldRotations, joint);
            const SkeletonGltfBasis restBasis{
                rotatedAxis(parentRelativeRest, Vec3{1.0f, 0.0f, 0.0f}),
                rotatedAxis(parentRelativeRest, Vec3{0.0f, 1.0f, 0.0f}),
                rotatedAxis(parentRelativeRest, Vec3{0.0f, 0.0f, 1.0f})
            };
            const Vec3 targetY = normalizeMappingVec3Or(
                skeletonGltfPrimaryBoneDirection(posedExportJoints, joint),
                restBasis.y
            );
            const SkeletonGltfBasis handBasis = basisFromYAndZHint(
                targetY,
                sourceSideAxes[index],
                restBasis
            );
            worldRotations[index] = previousWorld != nullptr ? closestSkeletonGltfRoll(handBasis, (*previousWorld)[index]) : skeletonGltfQuaternionFromBasis(handBasis);
            continue;
        }
        if (animationPose && isProfileFingerJoint(joint)) {
            const auto parentRelativeRest =
                parentRelativeRestRotation(rest, restWorldRotations, worldRotations, joint);
            const auto swing = skeletonGltfSwingBetween(
                rotatedAxis(parentRelativeRest, Vec3{0.0f, 1.0f, 0.0f}),
                fingerBoneDirection(posedExportJoints, joint)
            );
            worldRotations[index] = ovtr::normalizeQuaternion(
                ovtr::multiplyQuaternion(swing, parentRelativeRest)
            );
            continue;
        }
        if (isLegAimJoint(joint)) {
            const int sideIndex = firstChildIndex(worldJoints, joint);
            SkeletonGltfBasis legBasis = skeletonGltfExportBasisFor(posedExportJoints, joint);
            legBasis = skeletonGltfBasisFromYAndXHint(
                normalizeMappingVec3Or(skeletonGltfPrimaryBoneDirection(posedExportJoints, joint), legBasis.y),
                sourceSideAxes[static_cast<std::size_t>(sideIndex >= 0 ? sideIndex : joint)],
                legBasis
            );
            worldRotations[index] = previousWorld != nullptr ? closestSkeletonGltfRoll(legBasis, (*previousWorld)[index]) : skeletonGltfQuaternionFromBasis(legBasis);
            continue;
        }
        if (isArmAimJoint(joint)) {
            SkeletonGltfBasis armBasis = skeletonGltfExportBasisFor(posedExportJoints, joint);
            armBasis = skeletonGltfBasisFromYAndXHint(
                normalizeMappingVec3Or(skeletonGltfPrimaryBoneDirection(posedExportJoints, joint), armBasis.y),
                sourceSideAxes[index],
                armBasis
            );
            worldRotations[index] = previousWorld != nullptr ? closestSkeletonGltfRoll(armBasis, (*previousWorld)[index]) : skeletonGltfQuaternionFromBasis(armBasis);
            continue;
        }
        if (isFootJoint(joint)) {
            const int sideIndex = firstChildIndex(worldJoints, joint);
            const auto parentRelativeRest =
                parentRelativeRestRotation(rest, restWorldRotations, worldRotations, joint);
            const SkeletonGltfBasis rollBasis{
                rotatedAxis(parentRelativeRest, Vec3{1.0f, 0.0f, 0.0f}),
                rotatedAxis(parentRelativeRest, Vec3{0.0f, 1.0f, 0.0f}),
                rotatedAxis(parentRelativeRest, Vec3{0.0f, 0.0f, 1.0f})
            };
            const Vec3 targetY = normalizeMappingVec3Or(
                skeletonGltfPrimaryBoneDirection(posedExportJoints, joint),
                rollBasis.y
            );
            const SkeletonGltfBasis footBasis = skeletonGltfBasisFromYAndXHint(
                targetY,
                sourceSideAxes[static_cast<std::size_t>(sideIndex >= 0 ? sideIndex : joint)],
                rollBasis
            );
            worldRotations[index] = previousWorld != nullptr ? closestSkeletonGltfRoll(footBasis, (*previousWorld)[index]) : skeletonGltfQuaternionFromBasis(footBasis);
            continue;
        }
        SkeletonGltfBasis poseBasis = skeletonGltfExportBasisFor(posedExportJoints, joint);
        if (alignRollToRest) {
            poseBasis = alignSkeletonGltfBasisRoll(poseBasis, skeletonGltfExportBasisFor(restExportJoints, joint));
        }
        worldRotations[index] = previousWorld != nullptr && joint != kProfileJointSpine
            ? closestSkeletonGltfRoll(poseBasis, (*previousWorld)[index])
            : skeletonGltfQuaternionFromBasis(poseBasis);
    }
    SkeletonPose exportPose = sourcePose;
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        if (parent < 0) {
            exportPose.bones[static_cast<std::size_t>(joint)].localTranslationMeters =
                Vec3{};
            exportPose.bones[static_cast<std::size_t>(joint)].localRotation = {0.0f, 0.0f, 0.0f, 1.0f};
            continue;
        }
        if (joint == kProfileJointSpine) {
            exportPose.bones[static_cast<std::size_t>(joint)].localTranslationMeters =
                posedExportJoints[static_cast<std::size_t>(joint)].positionMeters;
            exportPose.bones[static_cast<std::size_t>(joint)].localRotation =
                worldRotations[static_cast<std::size_t>(joint)];
            continue;
        }
        const Vec3 delta = subMappingVec3(
            restPositions[static_cast<std::size_t>(joint)],
            restPositions[static_cast<std::size_t>(parent)]
        );
        exportPose.bones[static_cast<std::size_t>(joint)].localTranslationMeters =
            toLocalDelta(worldRotations[static_cast<std::size_t>(parent)], delta);
        exportPose.bones[static_cast<std::size_t>(joint)].localRotation = ovtr::normalizeQuaternion(
            ovtr::multiplyQuaternion(
                ovtr::conjugateQuaternion(worldRotations[static_cast<std::size_t>(parent)]),
                worldRotations[static_cast<std::size_t>(joint)]
            )
        );
    }
    return exportPose;
}

} // namespace

SkeletonPose makeSkeletonGltfExportPose(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose,
    const bool alignRollToRest
)
{
    return makeSkeletonGltfExportPoseInternal(rest, pose, alignRollToRest, nullptr);
}

std::vector<SkeletonPose> makeSkeletonGltfExportPoses(
    const ProfileSkeletonJoints& rest,
    const std::vector<SkeletonPose>& poses
) {
    std::vector<SkeletonPose> out;
    out.reserve(poses.size());
    SkeletonPose bindPose = makeSkeletonGltfExportPose(rest, makeRestSkeletonPose(rest), true);
    auto previousWorld = exportWorldRotations(rest, bindPose);
    for (const SkeletonPose& pose : poses) {
        out.push_back(makeSkeletonGltfExportPoseInternal(rest, pose, false, &previousWorld));
        previousWorld = exportWorldRotations(rest, out.back());
    }
    return out;
}

} // namespace ovtr::win32
