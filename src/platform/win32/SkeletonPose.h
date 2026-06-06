#pragma once

#include "platform/win32/ProfileSkeleton.h"

#include <array>

namespace ovtr::win32 {

struct SkeletonBonePose {
    Vec3 localTranslationMeters{};
    std::array<float, 4> localRotation{0.0f, 0.0f, 0.0f, 1.0f};
    bool valid = false;
};

struct SkeletonPose {
    double timeSeconds = 0.0;
    std::array<SkeletonBonePose, kProfileSkeletonJointCount> bones{};
};

SkeletonPose makeRestSkeletonPose(const ProfileSkeletonJoints& rest);
SkeletonPose makeSkeletonPoseFromWorldJoints(const ProfileSkeletonJoints& rest, const ProfileSkeletonJoints& pose);
void stabilizeSkeletonConnectorRolls(const ProfileSkeletonJoints& rest, SkeletonPose& pose);
ProfileSkeletonJoints computeSkeletonPoseWorldJoints(const ProfileSkeletonJoints& rest, const SkeletonPose& pose);
std::array<std::array<float, 4>, kProfileSkeletonJointCount> computeSkeletonPoseWorldRotations(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
);
std::array<Vec3, kProfileSkeletonJointCount> computeSkeletonPoseWorldSideAxes(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
);
std::array<Vec3, kProfileSkeletonJointCount> computeSkeletonPoseWorldForwardAxes(
    const ProfileSkeletonJoints& rest,
    const SkeletonPose& pose
);
Vec3 skeletonWorldAxis(const std::array<float, 4>& rotation, Vec3 localAxis) noexcept;

} // namespace ovtr::win32
