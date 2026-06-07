#include "platform/win32/MappingVmcFingerSolve.h"

#include "data/VmcSyntheticPose.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppStateConstants.h"
#include "platform/win32/MappingFingerSampling.h"
#include "platform/win32/MappingHandBasis.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingTransformMath.h"

#include <cstddef>

namespace ovtr::win32 {
namespace {

struct VmcPoseCache {
    std::array<MappingTransform, ovtr::kVmcFingerBoneCount> transforms{};
    std::array<bool, ovtr::kVmcFingerBoneCount> valid{};
};

ovtr::SkeletalHandSide skeletalSide(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? ovtr::SkeletalHandSide::Left : ovtr::SkeletalHandSide::Right;
}

int sideSlot(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? 0 : 1;
}

bool collectVmcPoseCache(
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const ProfileSkeletonHandSide side,
    VmcPoseCache& outCache
) {
    for (const ovtr::PoseSample& pose : poses.poses) {
        ovtr::SkeletalHandSide poseSide = ovtr::SkeletalHandSide::Left;
        std::uint32_t boneIndex = 0;
        if (!ovtr::decodeVmcFingerRuntimeIndex(pose.runtimeIndex, poseSide, boneIndex) ||
            poseSide != skeletalSide(side) ||
            (pose.flags & ovtr::PoseFlagPoseValid) == 0) {
            continue;
        }
        const ovtr::PoseSample displayPose =
            ovtr::applyOriginToPose(pose, originEnabled, originOffset, originRotationDegrees);
        outCache.transforms[boneIndex] = mappingTransformFromPose(displayPose);
        outCache.valid[boneIndex] = true;
    }
    return outCache.valid[0];
}

MappingFingerPolyline makeVmcFingerPolyline(const VmcPoseCache& cache, const ProfileSkeletonFinger finger)
{
    MappingFingerPolyline line;
    if (!cache.valid[0]) {
        return {};
    }
    line.points[line.count++] = cache.transforms[0].position;
    const std::uint32_t firstBone = 1u + static_cast<std::uint32_t>(finger) * 4u;
    for (std::uint32_t index = 0; index < 4u; ++index) {
        const std::uint32_t boneIndex = firstBone + index;
        if (!cache.valid[boneIndex]) {
            return {};
        }
        line.points[line.count++] = cache.transforms[boneIndex].position;
        if (line.count > 1) {
            line.length += distanceMappingVec3(line.points[line.count - 2], line.points[line.count - 1]);
        }
    }
    return line;
}

float distanceToVmcLinePoint(const MappingFingerPolyline& line, const int pointIndex) noexcept
{
    float distance = 0.0f;
    for (int index = 1; index <= pointIndex && index < line.count; ++index) {
        distance += distanceMappingVec3(
            line.points[static_cast<std::size_t>(index - 1)],
            line.points[static_cast<std::size_t>(index)]
        );
    }
    return distance;
}

float vmcSourceFingerDistanceForSegment(
    const MappingFingerPolyline& line,
    const ProfileSkeletonFinger finger,
    const float restDistance,
    const float restTotal,
    const int segment
) noexcept {
    if (line.count < 5) {
        return mappingSourceFingerDistanceForSegment(line, finger, restDistance, restTotal, segment);
    }
    const float proximalOriginDistance = distanceToVmcLinePoint(line, 1);
    const float deformLength = line.length - proximalOriginDistance;
    if (deformLength <= 0.00001f) {
        return mappingSourceFingerDistanceForSegment(line, finger, restDistance, restTotal, segment);
    }
    return proximalOriginDistance + deformLength * static_cast<float>(segment) /
        static_cast<float>(kProfileFingerJointSegments);
}

MappingFingerAlignment makeVmcFingerAlignment(
    const VmcPoseCache& cache,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonHandSide side
) noexcept {
    constexpr std::uint32_t kIndexBaseBone = 5;
    constexpr std::uint32_t kMiddleBaseBone = 9;
    constexpr std::uint32_t kPinkyBaseBone = 17;
    if (!cache.valid[0] || !cache.valid[kIndexBaseBone] ||
        !cache.valid[kMiddleBaseBone] || !cache.valid[kPinkyBaseBone]) {
        return {};
    }
    MappingFingerAlignment alignment;
    alignment.sourceToRoot = translationOnlySourceRoot(cache.transforms[0]);
    alignment.sourceWristRoot = transformMappingPoint(alignment.sourceToRoot, cache.transforms[0].position);
    alignment.sourceBasis = makeHandBasisFromPoints(
        alignment.sourceWristRoot,
        transformMappingPoint(alignment.sourceToRoot, cache.transforms[kMiddleBaseBone].position),
        transformMappingPoint(alignment.sourceToRoot, cache.transforms[kIndexBaseBone].position),
        transformMappingPoint(alignment.sourceToRoot, cache.transforms[kPinkyBaseBone].position),
        {}
    );
    mirrorRightHandSourceBasis(alignment.sourceBasis, side);
    const int restWrist = profileHandRootJoint(side);
    alignment.restBasis = makeHandBasisFromPoints(
        rest[static_cast<std::size_t>(restWrist)].positionMeters,
        rest[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Middle, 1))].positionMeters,
        rest[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Index, 1))].positionMeters,
        rest[static_cast<std::size_t>(profileFingerJoint(side, ProfileSkeletonFinger::Pinky, 1))].positionMeters,
        preferredPalmSideForHand(rest, side, restWrist)
    );
    orientHandBasisSide(alignment.restBasis, preferredPalmSideForHand(rest, side, restWrist));
    alignment.valid = alignment.sourceBasis.valid && alignment.restBasis.valid;
    return alignment;
}

bool applyVmcHand(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const VmcPoseCache& cache,
    const ProfileSkeletonHandSide side,
    const MappingTransform& targetWrist
) {
    const MappingFingerPolyline middle = makeVmcFingerPolyline(cache, ProfileSkeletonFinger::Middle);
    const float restMiddle = mappingRestFingerDistance(rest, side, ProfileSkeletonFinger::Middle, 4);
    if (middle.count < 2 || middle.length <= 0.00001f || restMiddle <= 0.00001f) {
        return false;
    }
    const MappingFingerAlignment alignment = makeVmcFingerAlignment(cache, rest, side);
    if (!alignment.valid) {
        return false;
    }
    const float scale = restMiddle / middle.length;
    const Vec3 palmSide = rotateMappingVector(targetWrist, alignment.restBasis.side);
    for (const ProfileSkeletonFinger finger : {
        ProfileSkeletonFinger::Thumb, ProfileSkeletonFinger::Index, ProfileSkeletonFinger::Middle,
        ProfileSkeletonFinger::Ring, ProfileSkeletonFinger::Pinky,
    }) {
        const MappingFingerPolyline line = makeVmcFingerPolyline(cache, finger);
        const float restTotal = mappingRestFingerDistance(rest, side, finger, 4);
        if (line.count < 2 || line.length <= 0.00001f || restTotal <= 0.00001f) {
            return false;
        }
        for (int segment = 1; segment <= kProfileFingerJointSegments; ++segment) {
            const float restDistance = mappingRestFingerDistance(rest, side, finger, segment);
            const float sourceDistance =
                vmcSourceFingerDistanceForSegment(line, finger, restDistance, restTotal, segment);
            const Vec3 local = alignSkeletalHandPoint(alignment, sampleMappingFingerPolyline(line, sourceDistance));
            const int joint = profileFingerJoint(side, finger, segment);
            out[static_cast<std::size_t>(joint)].positionMeters =
                transformMappingPoint(targetWrist, scaleMappingVec3(local, scale));
            out[static_cast<std::size_t>(joint)].sideHint = palmSide;
        }
    }
    return true;
}

} // namespace

bool mappingVmcFingerInputEnabledForSide(const MappingActor& actor, const ProfileSkeletonHandSide side) noexcept
{
    const std::uint32_t runtimeIndex = actor.mappingFingerRuntimeIndices[static_cast<std::size_t>(sideSlot(side))];
    ovtr::SkeletalHandSide selectedSide = ovtr::SkeletalHandSide::Left;
    std::uint32_t boneIndex = 0;
    return runtimeIndex != kNoSelectedRuntimeIndex &&
        ovtr::decodeVmcFingerRuntimeIndex(runtimeIndex, selectedSide, boneIndex) &&
        selectedSide == skeletalSide(side);
}

bool applyVmcFingerHandFromPoses(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees,
    const ProfileSkeletonHandSide side,
    const MappingTransform& targetWrist
) {
    VmcPoseCache cache;
    return collectVmcPoseCache(poses, originEnabled, originOffset, originRotationDegrees, side, cache) &&
        applyVmcHand(out, rest, cache, side, targetWrist);
}

} // namespace ovtr::win32
