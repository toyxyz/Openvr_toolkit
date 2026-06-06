#include "platform/win32/MappingLegRollHints.h"

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
    return std::atan2(dotMappingVec3(axis, cross(from, to)), dotMappingVec3(from, to));
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

Vec3 closestSideCandidate(const Vec3 axis, const Vec3 side, const Vec3 previous) noexcept
{
    const Vec3 projected = projectOff(axis, side, {1.0f, 0.0f, 0.0f});
    const Vec3 prevProjected = projectOff(axis, previous, projected);
    return dotMappingVec3(projected, prevProjected) >= 0.0f
        ? projected
        : scaleMappingVec3(projected, -1.0f);
}

Vec3 pelvisRestSegmentSide(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const ProfileSkeletonJoints& rest,
    const int segment
) noexcept {
    return rotateMappingVector(targetFor(targets, MappingTrackerRole::Pelvis), segmentSide(rest, segment));
}

Vec3 legSegmentSide(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& pose,
    const int segment,
    const bool left
) noexcept {
    const int hip = left ? kProfileJointLeftUpLeg : kProfileJointRightUpLeg;
    const int knee = left ? kProfileJointLeftLeg : kProfileJointRightLeg;
    const int ankle = left ? kProfileJointLeftFoot : kProfileJointRightFoot;
    const MappingTransform pelvis = targetFor(targets, MappingTrackerRole::Pelvis);
    const Vec3 restAxis = normalizeMappingVec3Or(rotateMappingVector(pelvis, segmentAxis(rest, segment)), {0.0f, 1.0f, 0.0f});
    const Vec3 restFallback = pelvisRestSegmentSide(targets, rest, segment);
    Vec3 restHinge = rotateMappingVector(pelvis, limbPlaneNormal(rest, hip, knee, ankle, restFallback));
    restHinge = projectOff(restAxis, restHinge, restFallback);
    const Vec3 restProfile = restHinge;
    const float offset = signedAngleAround(restAxis, restHinge, projectOff(restAxis, restProfile, restHinge));

    const Vec3 poseAxis = segmentAxis(pose, segment);
    Vec3 hinge = limbPlaneNormal(pose, hip, knee, ankle, restHinge);
    hinge = projectOff(poseAxis, hinge, restProfile);
    if (dotMappingVec3(hinge, restHinge) < 0.0f) {
        hinge = scaleMappingVec3(hinge, -1.0f);
    }
    return normalizeMappingVec3Or(rotateAroundAxis(hinge, poseAxis, offset), restProfile);
}

Vec3 footRollSide(
    const ProfileSkeletonJoints& pose,
    const int foot,
    const Vec3 side,
    const Vec3 previous
) noexcept {
    return closestSideCandidate(segmentAxis(pose, foot), side, previous);
}

Vec3 toeRollSide(
    const ProfileSkeletonJoints& pose,
    const int toe,
    const Vec3 side,
    const Vec3 previous
) noexcept {
    return closestSideCandidate(segmentAxis(pose, toe), side, previous);
}

} // namespace

void applyMappingLegRollHints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const std::array<Vec3, kProfileSkeletonJointCount>* previousSideAxes
) noexcept {
    const auto previous = [previousSideAxes](const int joint) noexcept {
        return previousSideAxes != nullptr ? (*previousSideAxes)[static_cast<std::size_t>(joint)] : Vec3{};
    };
    out[kProfileJointLeftUpLeg].sideHint = legSegmentSide(targets, rest, out, kProfileJointLeftUpLeg, true);
    out[kProfileJointRightUpLeg].sideHint = legSegmentSide(targets, rest, out, kProfileJointRightUpLeg, false);
    out[kProfileJointLeftLeg].sideHint = legSegmentSide(targets, rest, out, kProfileJointLeftLeg, true);
    out[kProfileJointRightLeg].sideHint = legSegmentSide(targets, rest, out, kProfileJointRightLeg, false);
    out[kProfileJointLeftFoot].sideHint =
        footRollSide(out, kProfileJointLeftFoot, out[kProfileJointLeftLeg].sideHint, previous(kProfileJointLeftFoot));
    out[kProfileJointRightFoot].sideHint =
        footRollSide(out, kProfileJointRightFoot, out[kProfileJointRightLeg].sideHint, previous(kProfileJointRightFoot));
    out[kProfileJointLeftToeBase].sideHint =
        toeRollSide(out, kProfileJointLeftToeBase, out[kProfileJointLeftFoot].sideHint, previous(kProfileJointLeftToeBase));
    out[kProfileJointRightToeBase].sideHint =
        toeRollSide(out, kProfileJointRightToeBase, out[kProfileJointRightFoot].sideHint, previous(kProfileJointRightToeBase));
}

} // namespace ovtr::win32
