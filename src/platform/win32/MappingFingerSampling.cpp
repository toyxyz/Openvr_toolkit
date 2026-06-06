#include "platform/win32/MappingFingerSampling.h"

#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr int kOpenVrWristBone = 1;

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

float nonThumbSampleRatio(const ProfileSkeletonFinger finger, const int segment) noexcept
{
    if (finger == ProfileSkeletonFinger::Pinky) {
        static constexpr std::array<float, kProfileFingerJointSegments> kPinkyRatios{
            0.5f, 0.78f, 0.9f, 1.0f
        };
        return kPinkyRatios[static_cast<std::size_t>(segment - 1)];
    }
    return static_cast<float>(segment + 2) / 6.0f;
}

} // namespace

MappingFingerPolyline makeMappingFingerPolyline(
    const std::array<MappingTransform, ovtr::kSkeletalHandBoneCount>& transforms,
    const std::array<bool, ovtr::kSkeletalHandBoneCount>& valid,
    const ProfileSkeletonFinger finger
) {
    MappingFingerPolyline line;
    line.points[0] = transforms[static_cast<std::size_t>(kOpenVrWristBone)].position;
    line.count = 1;
    for (const int boneIndex : sourceBonesForFinger(finger)) {
        if (boneIndex < 0) { continue; }
        if (!valid[static_cast<std::size_t>(boneIndex)]) { return MappingFingerPolyline{}; }
        line.points[static_cast<std::size_t>(line.count++)] =
            transforms[static_cast<std::size_t>(boneIndex)].position;
    }
    for (int index = 1; index < line.count; ++index) {
        line.length += distanceMappingVec3(
            line.points[static_cast<std::size_t>(index - 1)],
            line.points[static_cast<std::size_t>(index)]
        );
    }
    return line;
}

Vec3 sampleMappingFingerPolyline(const MappingFingerPolyline& line, const float distance) noexcept
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

float mappingRestFingerDistance(
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

float mappingSourceFingerDistanceForSegment(
    const MappingFingerPolyline& line,
    const ProfileSkeletonFinger finger,
    const float restDistance,
    const float restTotal,
    const int segment
) noexcept {
    if (finger == ProfileSkeletonFinger::Thumb || line.count < 2 || restTotal <= 0.00001f) {
        return line.length * restDistance / restTotal;
    }
    return line.length * nonThumbSampleRatio(finger, segment);
}

} // namespace ovtr::win32
