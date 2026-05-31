#include "platform/win32/MappingFingerSolve.h"

#include "data/SkeletalSyntheticPose.h"
#include "math/PoseTransform.h"
#include "platform/win32/MappingHandBasis.h"
#include "platform/win32/MappingTransformMath.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr int kHandSideCount = 2;
constexpr int kOpenVrWristBone = 1;
constexpr int kMaxSourcePoints = 6;

struct SkeletalPoseCache {
    std::array<MappingTransform, ovtr::kSkeletalHandBoneCount> transforms{};
    std::array<bool, ovtr::kSkeletalHandBoneCount> valid{};
};

struct SourcePolyline {
    std::array<Vec3, kMaxSourcePoints> points{};
    int count = 0;
    float length = 0.0f;
};

int sideSlot(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? 0 : 1;
}

MappingTrackerRole handRole(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? MappingTrackerRole::LeftHand : MappingTrackerRole::RightHand;
}

ovtr::SkeletalHandSide skeletalSide(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? ovtr::SkeletalHandSide::Left : ovtr::SkeletalHandSide::Right;
}

MappingTransform wristTargetFor(
    const ProfileSkeletonJoints& joints,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const ProfileSkeletonHandSide side
) noexcept {
    const MappingVirtualTarget& target =
        targets[static_cast<std::size_t>(mappingSlotForRole(handRole(side)))];
    MappingTransform wrist = target.transform;
    wrist.position = joints[static_cast<std::size_t>(profileWristJoint(side))].positionMeters;
    return wrist;
}

bool collectHandPoseCache(
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const ProfileSkeletonHandSide side,
    SkeletalPoseCache& outCache
) {
    for (const ovtr::PoseSample& pose : poses.poses) {
        ovtr::SkeletalHandSide poseSide = ovtr::SkeletalHandSide::Left;
        std::uint32_t boneIndex = 0;
        if (!ovtr::decodeSkeletalBoneRuntimeIndex(pose.runtimeIndex, poseSide, boneIndex) ||
            poseSide != skeletalSide(side) ||
            (pose.flags & ovtr::PoseFlagPoseValid) == 0) {
            continue;
        }
        const ovtr::PoseSample displayPose =
            ovtr::applyOriginToPose(pose, originEnabled, originOffset, originRotationDegrees);
        outCache.transforms[boneIndex] = mappingTransformFromPose(displayPose);
        outCache.valid[boneIndex] = true;
    }
    return outCache.valid[static_cast<std::size_t>(kOpenVrWristBone)];
}

std::array<int, 5> sourceBonesForFinger(const ProfileSkeletonFinger finger) noexcept
{
    switch (finger) {
    case ProfileSkeletonFinger::Thumb:
        return {2, 3, 4, 5, -1};
    case ProfileSkeletonFinger::Index:
        return {6, 7, 8, 9, 10};
    case ProfileSkeletonFinger::Middle:
        return {11, 12, 13, 14, 15};
    case ProfileSkeletonFinger::Ring:
        return {16, 17, 18, 19, 20};
    case ProfileSkeletonFinger::Pinky:
        return {21, 22, 23, 24, 25};
    }
    return {11, 12, 13, 14, 15};
}

SourcePolyline makePolyline(const SkeletalPoseCache& cache, const ProfileSkeletonFinger finger)
{
    SourcePolyline line;
    line.points[0] = cache.transforms[static_cast<std::size_t>(kOpenVrWristBone)].position;
    line.count = 1;
    for (const int boneIndex : sourceBonesForFinger(finger)) {
        if (boneIndex < 0) {
            continue;
        }
        if (!cache.valid[static_cast<std::size_t>(boneIndex)]) {
            return SourcePolyline{};
        }
        line.points[static_cast<std::size_t>(line.count++)] =
            cache.transforms[static_cast<std::size_t>(boneIndex)].position;
    }
    for (int index = 1; index < line.count; ++index) {
        line.length += distanceMappingVec3(
            line.points[static_cast<std::size_t>(index - 1)],
            line.points[static_cast<std::size_t>(index)]
        );
    }
    return line;
}

Vec3 samplePolyline(const SourcePolyline& line, const float distance) noexcept
{
    float remaining = distance;
    for (int index = 1; index < line.count; ++index) {
        const Vec3 start = line.points[static_cast<std::size_t>(index - 1)];
        const Vec3 end = line.points[static_cast<std::size_t>(index)];
        const float segmentLength = distanceMappingVec3(start, end);
        if (remaining <= segmentLength || index == line.count - 1) {
            const float t = segmentLength > 0.00001f ? remaining / segmentLength : 0.0f;
            return addMappingVec3(start, scaleMappingVec3(subMappingVec3(end, start), t));
        }
        remaining -= segmentLength;
    }
    return line.points[static_cast<std::size_t>(line.count - 1)];
}

float restFingerDistance(
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonHandSide side,
    const ProfileSkeletonFinger finger,
    const int segment
) noexcept {
    int parent = profileHandRootJoint(side);
    float distance = 0.0f;
    for (int current = 1; current <= segment; ++current) {
        const int joint = profileFingerJoint(side, finger, current);
        distance += distanceMappingVec3(
            rest[static_cast<std::size_t>(parent)].positionMeters,
            rest[static_cast<std::size_t>(joint)].positionMeters
        );
        parent = joint;
    }
    return distance;
}

bool applySkeletalHand(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const SkeletalPoseCache& cache,
    const ProfileSkeletonHandSide side,
    const MappingTransform& targetWrist
) {
    const SourcePolyline middle = makePolyline(cache, ProfileSkeletonFinger::Middle);
    const float restMiddle = restFingerDistance(rest, side, ProfileSkeletonFinger::Middle, 4);
    if (middle.count < 2 || middle.length <= 0.00001f || restMiddle <= 0.00001f) {
        return false;
    }

    const MappingFingerAlignment alignment = makeMappingFingerAlignment(cache.transforms, cache.valid, rest, side);
    if (!alignment.valid) {
        return false;
    }
    const float scale = restMiddle / middle.length;
    for (const ProfileSkeletonFinger finger : {
        ProfileSkeletonFinger::Thumb,
        ProfileSkeletonFinger::Index,
        ProfileSkeletonFinger::Middle,
        ProfileSkeletonFinger::Ring,
        ProfileSkeletonFinger::Pinky,
    }) {
        const SourcePolyline line = makePolyline(cache, finger);
        const float restTotal = restFingerDistance(rest, side, finger, 4);
        if (line.count < 2 || line.length <= 0.00001f || restTotal <= 0.00001f) {
            return false;
        }
        for (int segment = 1; segment <= kProfileFingerJointSegments; ++segment) {
            const float sourceDistance = line.length * restFingerDistance(rest, side, finger, segment) / restTotal;
            const Vec3 local = alignSkeletalHandPoint(alignment, samplePolyline(line, sourceDistance));
            const int joint = profileFingerJoint(side, finger, segment);
            out[static_cast<std::size_t>(joint)].positionMeters =
                transformMappingPoint(targetWrist, scaleMappingVec3(local, scale));
            out[static_cast<std::size_t>(joint)].sideHint =
                rotateMappingVector(targetWrist, rest[static_cast<std::size_t>(joint)].sideHint);
        }
    }
    return true;
}

void copyPreviousHand(
    const MappingActor& actor,
    ProfileSkeletonJoints& out,
    const ProfileSkeletonHandSide side
) noexcept {
    if (!actor.liveJointsValid || !actor.liveFingerJointsValid[static_cast<std::size_t>(sideSlot(side))]) {
        return;
    }
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const bool left = index == kProfileJointLeftHand ||
            (index >= kProfileJointLeftHandThumb1 && index <= kProfileJointLeftHandPinky4);
        const bool right = index == kProfileJointRightHand ||
            (index >= kProfileJointRightHandThumb1 && index <= kProfileJointRightHandPinky4);
        if ((side == ProfileSkeletonHandSide::Left && left) ||
            (side == ProfileSkeletonHandSide::Right && right)) {
            out[static_cast<std::size_t>(index)].positionMeters =
                actor.liveJoints[static_cast<std::size_t>(index)].positionMeters;
            out[static_cast<std::size_t>(index)].sideHint =
                actor.liveJoints[static_cast<std::size_t>(index)].sideHint;
        }
    }
}

} // namespace

void applyRestHandFingerJoints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const MappingTransform& wristTarget,
    const ProfileSkeletonHandSide side
) noexcept {
    const int wrist = profileWristJoint(side);
    for (int index = 0; index < kProfileSkeletonJointCount; ++index) {
        const bool applies = (side == ProfileSkeletonHandSide::Left &&
            (index == kProfileJointLeftHand || (index >= kProfileJointLeftHandThumb1 && index <= kProfileJointLeftHandPinky4))) ||
            (side == ProfileSkeletonHandSide::Right &&
            (index == kProfileJointRightHand || (index >= kProfileJointRightHandThumb1 && index <= kProfileJointRightHandPinky4)));
        if (!applies) {
            continue;
        }
        const Vec3 delta = subMappingVec3(
            rest[static_cast<std::size_t>(index)].positionMeters,
            rest[static_cast<std::size_t>(wrist)].positionMeters
        );
        out[static_cast<std::size_t>(index)].positionMeters = transformMappingPoint(wristTarget, delta);
        out[static_cast<std::size_t>(index)].sideHint =
            rotateMappingVector(wristTarget, rest[static_cast<std::size_t>(index)].sideHint);
    }
}

void updateMappingActorFingerJoints(
    MappingActor& actor,
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets
) {
    for (const ProfileSkeletonHandSide side : {ProfileSkeletonHandSide::Left, ProfileSkeletonHandSide::Right}) {
        const MappingTransform wristTarget = wristTargetFor(out, targets, side);
        applyRestHandFingerJoints(out, rest, wristTarget, side);
        SkeletalPoseCache cache;
        const bool hasHand = collectHandPoseCache(
            poses,
            originEnabled,
            originOffset,
            originRotationDegrees,
            side,
            cache
        );
        const bool updated = hasHand && applySkeletalHand(out, rest, cache, side, wristTarget);
        actor.liveFingerJointsValid[static_cast<std::size_t>(sideSlot(side))] = updated ||
            actor.liveFingerJointsValid[static_cast<std::size_t>(sideSlot(side))];
        if (!updated) {
            copyPreviousHand(actor, out, side);
        }
    }
}

} // namespace ovtr::win32
