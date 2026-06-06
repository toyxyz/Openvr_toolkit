#include "platform/win32/MappingArmRollHints.h"

#include "platform/win32/MappingTransformMath.h"

#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

MappingTransform targetFor(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    return targets[static_cast<std::size_t>(mappingSlotForRole(role))].transform;
}

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec3 projectOff(const Vec3 axis, const Vec3 value, const Vec3 fallback) noexcept
{
    const Vec3 projected = subMappingVec3(value, scaleMappingVec3(axis, dotMappingVec3(value, axis)));
    return normalizeMappingVec3Or(projected, fallback);
}

Vec3 segmentAxis(const ProfileSkeletonJoints& joints, const int segment) noexcept
{
    const int parent = joints[static_cast<std::size_t>(segment)].parentIndex;
    return parent >= 0
        ? normalizeMappingVec3Or(subMappingVec3(joints[segment].positionMeters, joints[parent].positionMeters), {0.0f, 1.0f, 0.0f})
        : Vec3{0.0f, 1.0f, 0.0f};
}

Vec3 segmentSide(const ProfileSkeletonJoints& joints, const int segment) noexcept
{
    const Vec3 axis = segmentAxis(joints, segment);
    const Vec3 fallback = std::fabs(axis.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
    return projectOff(axis, joints[static_cast<std::size_t>(segment)].sideHint, normalizeMappingVec3Or(cross(axis, fallback), {1.0f, 0.0f, 0.0f}));
}

Vec3 limbPlaneNormal(
    const ProfileSkeletonJoints& joints,
    const int root,
    const int mid,
    const int end,
    const Vec3 fallback
) noexcept {
    const Vec3 upper = subMappingVec3(joints[mid].positionMeters, joints[root].positionMeters);
    const Vec3 lower = subMappingVec3(joints[end].positionMeters, joints[mid].positionMeters);
    return normalizeMappingVec3Or(cross(upper, lower), fallback);
}

float signedAngleAround(const Vec3 axis, const Vec3 from, const Vec3 to) noexcept
{
    const float s = dotMappingVec3(axis, cross(from, to));
    const float c = dotMappingVec3(from, to);
    return std::atan2(s, c);
}

Vec3 rotateAroundAxis(const Vec3 value, const Vec3 axis, const float radians) noexcept
{
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return addMappingVec3(
        addMappingVec3(scaleMappingVec3(value, c), scaleMappingVec3(cross(axis, value), s)),
        scaleMappingVec3(axis, dotMappingVec3(axis, value) * (1.0f - c))
    );
}

Vec3 chooseContinuousSide(const Vec3 axis, const Vec3 side, const Vec3 reference) noexcept
{
    const Vec3 projected = projectOff(axis, reference, side);
    return dotMappingVec3(side, projected) < 0.0f ? scaleMappingVec3(side, -1.0f) : side;
}

Vec3 chestRestSegmentSide(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const ProfileSkeletonJoints& rest,
    const int segment
) noexcept {
    return rotateMappingVector(targetFor(targets, MappingTrackerRole::Chest), segmentSide(rest, segment));
}

Vec3 armSegmentSide(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& pose,
    const int segment,
    const bool left,
    const Vec3 previousSide
) noexcept {
    const int shoulder = left ? kProfileJointLeftShoulder : kProfileJointRightShoulder;
    const int elbow = left ? kProfileJointLeftArm : kProfileJointRightArm;
    const int wrist = left ? kProfileJointLeftForeArm : kProfileJointRightForeArm;
    const MappingTransform chest = targetFor(targets, MappingTrackerRole::Chest);
    const Vec3 restAxis = normalizeMappingVec3Or(rotateMappingVector(chest, segmentAxis(rest, segment)), {0.0f, 1.0f, 0.0f});
    const Vec3 restProfile = chestRestSegmentSide(targets, rest, segment);
    Vec3 restHinge = rotateMappingVector(chest, limbPlaneNormal(rest, shoulder, elbow, wrist, restProfile));
    restHinge = projectOff(restAxis, restHinge, restProfile);
    const float offset = signedAngleAround(restAxis, restHinge, projectOff(restAxis, restProfile, restHinge));

    const Vec3 poseAxis = segmentAxis(pose, segment);
    Vec3 hinge = limbPlaneNormal(pose, shoulder, elbow, wrist, restHinge);
    hinge = projectOff(poseAxis, hinge, restProfile);
    const Vec3 side = normalizeMappingVec3Or(rotateAroundAxis(hinge, poseAxis, offset), restProfile);
    const Vec3 previous = lengthMappingVec3(previousSide) > 0.0001f
        ? previousSide
        : restProfile;
    return chooseContinuousSide(poseAxis, side, previous);
}

} // namespace

void applyMappingArmRollHints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const std::array<Vec3, kProfileSkeletonJointCount>* previousSideAxes
) noexcept {
    const auto previous = [previousSideAxes](const int joint) noexcept {
        return previousSideAxes != nullptr ? (*previousSideAxes)[static_cast<std::size_t>(joint)] : Vec3{};
    };
    out[kProfileJointLeftArm].sideHint =
        armSegmentSide(targets, rest, out, kProfileJointLeftArm, true, previous(kProfileJointLeftArm));
    out[kProfileJointRightArm].sideHint =
        armSegmentSide(targets, rest, out, kProfileJointRightArm, false, previous(kProfileJointRightArm));
    out[kProfileJointLeftForeArm].sideHint =
        armSegmentSide(targets, rest, out, kProfileJointLeftForeArm, true, previous(kProfileJointLeftForeArm));
    out[kProfileJointRightForeArm].sideHint =
        armSegmentSide(targets, rest, out, kProfileJointRightForeArm, false, previous(kProfileJointRightForeArm));
}

} // namespace ovtr::win32
