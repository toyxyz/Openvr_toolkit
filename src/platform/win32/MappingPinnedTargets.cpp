#include "platform/win32/MappingPinnedTargets.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingTransformMath.h"

#include <cstddef>

namespace ovtr::win32 {
namespace {

MappingTransform targetFor(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    return targets[static_cast<std::size_t>(mappingSlotForRole(role))].transform;
}

Vec3 toVec3(const std::array<float, 3>& value) noexcept
{
    return Vec3{value[0], value[1], value[2]};
}

std::array<float, 3> toArray(const Vec3 value) noexcept
{
    return {value.x, value.y, value.z};
}

Vec3 restDelta(const ProfileSkeletonJoints& rest, const int child, const int parent) noexcept
{
    return subMappingVec3(rest[child].positionMeters, rest[parent].positionMeters);
}

void setLocalRotationForWorld(
    const ProfileSkeletonJoints& rest,
    const int joint,
    const std::array<float, 4>& desiredWorldRotation,
    SkeletonPose& pose
) {
    const auto worldRotations = computeSkeletonPoseWorldRotations(rest, pose);
    const int parent = rest[static_cast<std::size_t>(joint)].parentIndex;
    pose.bones[static_cast<std::size_t>(joint)].localRotation = parent >= 0
        ? ovtr::normalizeQuaternion(ovtr::multiplyQuaternion(
            ovtr::conjugateQuaternion(worldRotations[static_cast<std::size_t>(parent)]),
            desiredWorldRotation
        ))
        : desiredWorldRotation;
}

void setLocalTranslationForWorld(
    const ProfileSkeletonJoints& rest,
    const int joint,
    const Vec3 desiredWorldPosition,
    SkeletonPose& pose
) {
    const auto worldRotations = computeSkeletonPoseWorldRotations(rest, pose);
    const ProfileSkeletonJoints worldJoints = computeSkeletonPoseWorldJoints(rest, pose);
    const int parent = rest[static_cast<std::size_t>(joint)].parentIndex;
    if (parent < 0) {
        pose.bones[static_cast<std::size_t>(joint)].localTranslationMeters = desiredWorldPosition;
        return;
    }
    const Vec3 delta = subMappingVec3(
        desiredWorldPosition,
        worldJoints[static_cast<std::size_t>(parent)].positionMeters
    );
    pose.bones[static_cast<std::size_t>(joint)].localTranslationMeters = toVec3(
        ovtr::rotatePositionByQuaternion(
            ovtr::conjugateQuaternion(worldRotations[static_cast<std::size_t>(parent)]),
            toArray(delta)
        )
    );
}

void pinJointRotation(
    const ProfileSkeletonJoints& rest,
    const int joint,
    const MappingTransform& target,
    SkeletonPose& pose
) {
    setLocalRotationForWorld(rest, joint, ovtr::normalizeQuaternion(target.rotation), pose);
}

void pinFoot(
    const ProfileSkeletonJoints& rest,
    const int foot,
    const int toe,
    const MappingTransform& target,
    SkeletonPose& pose
) {
    pinJointRotation(rest, foot, target, pose);
    setLocalTranslationForWorld(rest, toe, transformMappingPoint(target, restDelta(rest, toe, foot)), pose);
}

} // namespace

void applyPinnedMappingTargets(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    SkeletonPose& pose
) {
    if (calibration.pinHandTargets) {
        pinJointRotation(rest, kProfileJointLeftHand, targetFor(targets, MappingTrackerRole::LeftHand), pose);
        pinJointRotation(rest, kProfileJointRightHand, targetFor(targets, MappingTrackerRole::RightHand), pose);
    }
    if (calibration.pinFootTargets) {
        pinFoot(rest, kProfileJointLeftFoot, kProfileJointLeftToeBase, targetFor(targets, MappingTrackerRole::LeftFoot), pose);
        pinFoot(rest, kProfileJointRightFoot, kProfileJointRightToeBase, targetFor(targets, MappingTrackerRole::RightFoot), pose);
    }
}

} // namespace ovtr::win32
