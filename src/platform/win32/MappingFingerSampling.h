#pragma once

#include "data/SkeletalSyntheticPose.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/ProfileSkeleton.h"

#include <array>

namespace ovtr::win32 {

constexpr int kMappingFingerSourcePointCount = 6;

struct MappingFingerPolyline {
    std::array<Vec3, kMappingFingerSourcePointCount> points{};
    int count = 0;
    float length = 0.0f;
};

MappingFingerPolyline makeMappingFingerPolyline(
    const std::array<MappingTransform, ovtr::kSkeletalHandBoneCount>& transforms,
    const std::array<bool, ovtr::kSkeletalHandBoneCount>& valid,
    ProfileSkeletonFinger finger
);
Vec3 sampleMappingFingerPolyline(const MappingFingerPolyline& line, float distance) noexcept;
float mappingRestFingerDistance(
    const ProfileSkeletonJoints& rest,
    ProfileSkeletonHandSide side,
    ProfileSkeletonFinger finger,
    int segment
) noexcept;
float mappingSourceFingerDistanceForSegment(
    const MappingFingerPolyline& line,
    ProfileSkeletonFinger finger,
    float restDistance,
    float restTotal,
    int segment
) noexcept;

} // namespace ovtr::win32
