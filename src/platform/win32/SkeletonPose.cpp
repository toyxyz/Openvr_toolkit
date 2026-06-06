#include "platform/win32/SkeletonPose.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace ovtr::win32 {
namespace {

struct Basis {
    Vec3 x{};
    Vec3 y{};
    Vec3 z{};
};

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

int firstChildIndex(const ProfileSkeletonJoints& joints, const int parent) noexcept
{
    for (std::size_t index = 0; index < joints.size(); ++index) {
        if (joints[index].parentIndex == parent) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

Vec3 fallbackSideFor(const Vec3 y) noexcept
{
    return std::fabs(y.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{0.0f, 0.0f, 1.0f};
}

Vec3 rootUpFor(const ProfileSkeletonJoint& joint) noexcept
{
    return normalizeMappingVec3Or(joint.upHint, Vec3{0.0f, 1.0f, 0.0f});
}

Vec3 legPlaneSideFor(const ProfileSkeletonJoints& joints, const int index) noexcept
{
    const bool left = index == kProfileJointLeftUpLeg || index == kProfileJointLeftLeg || index == kProfileJointLeftFoot;
    const int hip = left ? kProfileJointLeftUpLeg : kProfileJointRightUpLeg;
    const int knee = left ? kProfileJointLeftLeg : kProfileJointRightLeg;
    const int ankle = left ? kProfileJointLeftFoot : kProfileJointRightFoot;
    const Vec3 upper = subMappingVec3(joints[static_cast<std::size_t>(knee)].positionMeters, joints[static_cast<std::size_t>(hip)].positionMeters);
    const Vec3 lower = subMappingVec3(joints[static_cast<std::size_t>(ankle)].positionMeters, joints[static_cast<std::size_t>(knee)].positionMeters);
    return normalizeMappingVec3Or(cross(upper, lower), Vec3{1.0f, 0.0f, 0.0f});
}

bool isLegRollJoint(const int index) noexcept
{
    return index == kProfileJointLeftUpLeg || index == kProfileJointLeftLeg || index == kProfileJointLeftFoot ||
        index == kProfileJointRightUpLeg || index == kProfileJointRightLeg || index == kProfileJointRightFoot;
}

Basis jointBasis(const ProfileSkeletonJoints& joints, const int index) noexcept
{
    const int child = firstChildIndex(joints, index);
    const int parent = joints[static_cast<std::size_t>(index)].parentIndex;
    Vec3 y = parent >= 0
        ? subMappingVec3(joints[static_cast<std::size_t>(index)].positionMeters, joints[static_cast<std::size_t>(parent)].positionMeters)
        : rootUpFor(joints[static_cast<std::size_t>(index)]);
    if (lengthMappingVec3(y) <= 0.0001f && child >= 0) {
        y = subMappingVec3(
            joints[static_cast<std::size_t>(child)].positionMeters,
            joints[static_cast<std::size_t>(index)].positionMeters
        );
    }
    y = normalizeMappingVec3Or(y, Vec3{0.0f, 1.0f, 0.0f});
    Vec3 x = joints[static_cast<std::size_t>(index)].sideHint;
    if (lengthMappingVec3(x) <= 0.0001f && isLegRollJoint(index)) {
        x = legPlaneSideFor(joints, index);
    }
    x = subMappingVec3(x, scaleMappingVec3(y, dotMappingVec3(x, y)));
    x = normalizeMappingVec3Or(x, normalizeMappingVec3Or(cross(y, fallbackSideFor(y)), Vec3{1.0f, 0.0f, 0.0f}));
    const Vec3 z = normalizeMappingVec3Or(cross(x, y), Vec3{0.0f, 0.0f, 1.0f});
    return {x, y, z};
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

Vec3 restLocalTranslation(const ProfileSkeletonJoints& rest, const int index) noexcept
{
    const int parent = rest[static_cast<std::size_t>(index)].parentIndex;
    if (parent < 0) {
        return rest[static_cast<std::size_t>(index)].positionMeters;
    }
    return subMappingVec3(
        rest[static_cast<std::size_t>(index)].positionMeters,
        rest[static_cast<std::size_t>(parent)].positionMeters
    );
}

std::array<Vec3, kProfileSkeletonJointCount> computeWorldBasisAxis(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose,
    const int axis
)
{
    std::array<Vec3, kProfileSkeletonJointCount> axes{};
    const auto rotations = computeSkeletonPoseWorldRotations(rest, pose);
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const Basis basis = jointBasis(rest, index);
        const Vec3 localAxis = axis == 0 ? basis.x : basis.z;
        axes[static_cast<std::size_t>(index)] =
            skeletonWorldAxis(rotations[static_cast<std::size_t>(index)], localAxis);
    }
    return axes;
}

} // namespace

SkeletonPose makeRestSkeletonPose(const ProfileSkeletonJoints& rest)
{
    SkeletonPose pose;
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        pose.bones[static_cast<std::size_t>(index)].localTranslationMeters = restLocalTranslation(rest, index);
        pose.bones[static_cast<std::size_t>(index)].valid = true;
    }
    return pose;
}

SkeletonPose makeSkeletonPoseFromWorldJoints(const ProfileSkeletonJoints& rest, const ProfileSkeletonJoints& poseJoints)
{
    SkeletonPose pose = makeRestSkeletonPose(rest);
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const std::size_t poseIndex = static_cast<std::size_t>(index);
        world[poseIndex] = quaternionFromBasisDelta(jointBasis(rest, index), jointBasis(poseJoints, index));
        const int parent = rest[static_cast<std::size_t>(index)].parentIndex;
        if (parent >= 0) {
            const Vec3 delta = subMappingVec3(
                poseJoints[static_cast<std::size_t>(index)].positionMeters,
                poseJoints[static_cast<std::size_t>(parent)].positionMeters
            );
            const std::array<float, 3> local = ovtr::rotatePositionByQuaternion(
                ovtr::conjugateQuaternion(world[static_cast<std::size_t>(parent)]),
                {delta.x, delta.y, delta.z}
            );
            pose.bones[static_cast<std::size_t>(index)].localTranslationMeters =
                Vec3{local[0], local[1], local[2]};
        }
        pose.bones[static_cast<std::size_t>(index)].localRotation = parent >= 0
            ? ovtr::multiplyQuaternion(ovtr::conjugateQuaternion(world[static_cast<std::size_t>(parent)]), world[static_cast<std::size_t>(index)])
            : world[static_cast<std::size_t>(index)];
        pose.bones[static_cast<std::size_t>(index)].valid = true;
    }
    pose.bones[kProfileJointHips].localTranslationMeters = poseJoints[kProfileJointHips].positionMeters;
    return pose;
}

void stabilizeSkeletonConnectorRolls(const ProfileSkeletonJoints& rest, SkeletonPose& pose)
{
    (void)rest;
    (void)pose;
    // Roll is owned by the solved limb side hints; connector overwrites break pole-following limbs.
}

std::array<std::array<float, 4>, kProfileSkeletonJointCount> computeSkeletonPoseWorldRotations(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
)
{
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const int parent = rest[static_cast<std::size_t>(index)].parentIndex;
        world[static_cast<std::size_t>(index)] = parent >= 0
            ? ovtr::multiplyQuaternion(world[static_cast<std::size_t>(parent)], pose.bones[static_cast<std::size_t>(index)].localRotation)
            : pose.bones[static_cast<std::size_t>(index)].localRotation;
    }
    return world;
}

ProfileSkeletonJoints computeSkeletonPoseWorldJoints(const ProfileSkeletonJoints& rest, const SkeletonPose& pose)
{
    ProfileSkeletonJoints joints = rest;
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> rotations{};
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const int parent = rest[static_cast<std::size_t>(index)].parentIndex;
        const SkeletonBonePose& bone = pose.bones[static_cast<std::size_t>(index)];
        rotations[static_cast<std::size_t>(index)] = parent >= 0
            ? ovtr::multiplyQuaternion(rotations[static_cast<std::size_t>(parent)], bone.localRotation)
            : bone.localRotation;
        if (parent < 0) {
            joints[static_cast<std::size_t>(index)].positionMeters = bone.localTranslationMeters;
        } else {
            const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(
                rotations[static_cast<std::size_t>(parent)],
                {bone.localTranslationMeters.x, bone.localTranslationMeters.y, bone.localTranslationMeters.z}
            );
            joints[static_cast<std::size_t>(index)].positionMeters = addMappingVec3(
                joints[static_cast<std::size_t>(parent)].positionMeters,
                Vec3{rotated[0], rotated[1], rotated[2]}
            );
        }
        joints[static_cast<std::size_t>(index)].sideHint =
            skeletonWorldAxis(rotations[static_cast<std::size_t>(index)], jointBasis(rest, index).x);
        joints[static_cast<std::size_t>(index)].upHint =
            skeletonWorldAxis(rotations[static_cast<std::size_t>(index)], jointBasis(rest, index).y);
    }
    return joints;
}

std::array<Vec3, kProfileSkeletonJointCount> computeSkeletonPoseWorldSideAxes(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
)
{
    return computeWorldBasisAxis(rest, pose, 0);
}

std::array<Vec3, kProfileSkeletonJointCount> computeSkeletonPoseWorldForwardAxes(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
)
{
    return computeWorldBasisAxis(rest, pose, 2);
}

Vec3 skeletonWorldAxis(const std::array<float, 4>& rotation, const Vec3 localAxis) noexcept
{
    const std::array<float, 3> axis = ovtr::rotatePositionByQuaternion(
        rotation,
        {localAxis.x, localAxis.y, localAxis.z}
    );
    return Vec3{axis[0], axis[1], axis[2]};
}

} // namespace ovtr::win32
