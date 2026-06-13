#include "platform/win32/MappingCoreSolve.h"

#include "math/PoseTransform.h"
#include "platform/win32/MappingTransformMath.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

MappingTransform targetFor(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    return targets[static_cast<std::size_t>(mappingSlotForRole(role))].transform;
}

Vec3 restDelta(const ProfileSkeletonJoints& joints, const int child, const int parent) noexcept
{
    return subMappingVec3(joints[child].positionMeters, joints[parent].positionMeters);
}

Vec3 rotateVector(const MappingTransform& transform, const Vec3 vector) noexcept
{
    const std::array<float, 3> rotated =
        ovtr::rotatePositionByQuaternion(transform.rotation, {vector.x, vector.y, vector.z});
    return Vec3{rotated[0], rotated[1], rotated[2]};
}

float restYRatio(const ProfileSkeletonJoints& joints, const int joint, const int start, const int end) noexcept
{
    const float denominator = joints[end].positionMeters.y - joints[start].positionMeters.y;
    if (denominator == 0.0f) {
        return 0.0f;
    }
    return (joints[joint].positionMeters.y - joints[start].positionMeters.y) / denominator;
}

MappingTransform yawTransformAt(const MappingTransform& transform) noexcept
{
    return MappingTransform{
        transform.position,
        ovtr::quaternionFromEulerDegrees({0.0f, ovtr::yawDegreesFromQuaternion(transform.rotation), 0.0f})
    };
}

Vec3 spineCurvePoint(
    const ProfileSkeletonJoints& rest,
    const int joint,
    const MappingTransform& hips,
    const MappingTransform& chest
) noexcept {
    const float t = restYRatio(rest, joint, kProfileJointHips, kProfileJointSpine2);
    const float t2 = t * t;
    const float t3 = t2 * t;
    const Vec3 p0 = hips.position;
    const Vec3 p1 = chest.position;
    const Vec3 m0 = rotateVector(hips, restDelta(rest, kProfileJointSpine2, kProfileJointHips));
    const Vec3 m1 = rotateVector(chest, restDelta(rest, kProfileJointSpine2, kProfileJointHips));
    return addMappingVec3(
        addMappingVec3(scaleMappingVec3(p0, 2.0f * t3 - 3.0f * t2 + 1.0f), scaleMappingVec3(m0, t3 - 2.0f * t2 + t)),
        addMappingVec3(scaleMappingVec3(p1, -2.0f * t3 + 3.0f * t2), scaleMappingVec3(m1, t3 - t2))
    );
}

} // namespace

MappingCoreSolveResult solveMappingCoreJoints(
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    ProfileSkeletonJoints& out
) {
    const MappingTransform head = targetFor(targets, MappingTrackerRole::Head);
    const MappingTransform chest = targetFor(targets, MappingTrackerRole::Chest);
    const MappingTransform pelvis = targetFor(targets, MappingTrackerRole::Pelvis);
    const MappingTransform pelvisYaw = yawTransformAt(pelvis);

    out[kProfileJointHips].positionMeters = pelvis.position;
    out[kProfileJointSpine2].positionMeters = chest.position;
    out[kProfileJointHead].positionMeters = head.position;
    out[kProfileJointSpine].positionMeters =
        transformMappingPoint(pelvis, restDelta(rest, kProfileJointSpine, kProfileJointHips));
    out[kProfileJointSpine1].positionMeters = spineCurvePoint(rest, kProfileJointSpine1, pelvisYaw, chest);

    MappingCoreSolveResult result;
    out[kProfileJointNeck].positionMeters =
        transformMappingPoint(head, restDelta(rest, kProfileJointNeck, kProfileJointHead));
    out[kProfileJointHeadTopEnd].positionMeters =
        transformMappingPoint(head, restDelta(rest, kProfileJointHeadTopEnd, kProfileJointHead));
    out[kProfileJointLeftShoulder].positionMeters =
        transformMappingPoint(chest, restDelta(rest, kProfileJointLeftShoulder, kProfileJointSpine2));
    out[kProfileJointRightShoulder].positionMeters =
        transformMappingPoint(chest, restDelta(rest, kProfileJointRightShoulder, kProfileJointSpine2));
    out[kProfileJointLeftUpLeg].positionMeters =
        transformMappingPoint(pelvis, restDelta(rest, kProfileJointLeftUpLeg, kProfileJointHips));
    out[kProfileJointRightUpLeg].positionMeters =
        transformMappingPoint(pelvis, restDelta(rest, kProfileJointRightUpLeg, kProfileJointHips));
    return result;
}

} // namespace ovtr::win32
