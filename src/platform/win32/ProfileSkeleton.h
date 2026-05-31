#pragma once

#include "platform/win32/ProfileModel.h"
#include "platform/win32/ViewportMath.h"

#include <array>

namespace ovtr::win32 {

inline constexpr int kProfileFingerJointSegments = 4;
inline constexpr int kProfileSkeletonJointCount = 63;

enum class ProfileSkeletonHandSide {
    Left,
    Right,
};

enum class ProfileSkeletonFinger {
    Thumb,
    Index,
    Middle,
    Ring,
    Pinky,
};

enum ProfileSkeletonJointIndex {
    kProfileJointHips,
    kProfileJointSpine,
    kProfileJointSpine1,
    kProfileJointSpine2,
    kProfileJointNeck,
    kProfileJointHead,
    kProfileJointHeadTopEnd,
    kProfileJointLeftShoulder,
    kProfileJointLeftArm,
    kProfileJointLeftForeArm,
    kProfileJointLeftHand,
    kProfileJointRightShoulder,
    kProfileJointRightArm,
    kProfileJointRightForeArm,
    kProfileJointRightHand,
    kProfileJointLeftUpLeg,
    kProfileJointLeftLeg,
    kProfileJointLeftFoot,
    kProfileJointLeftToeBase,
    kProfileJointRightUpLeg,
    kProfileJointRightLeg,
    kProfileJointRightFoot,
    kProfileJointRightToeBase,
    kProfileJointLeftHandThumb1,
    kProfileJointLeftHandThumb2,
    kProfileJointLeftHandThumb3,
    kProfileJointLeftHandThumb4,
    kProfileJointLeftHandIndex1,
    kProfileJointLeftHandIndex2,
    kProfileJointLeftHandIndex3,
    kProfileJointLeftHandIndex4,
    kProfileJointLeftHandMiddle1,
    kProfileJointLeftHandMiddle2,
    kProfileJointLeftHandMiddle3,
    kProfileJointLeftHandMiddle4,
    kProfileJointLeftHandRing1,
    kProfileJointLeftHandRing2,
    kProfileJointLeftHandRing3,
    kProfileJointLeftHandRing4,
    kProfileJointLeftHandPinky1,
    kProfileJointLeftHandPinky2,
    kProfileJointLeftHandPinky3,
    kProfileJointLeftHandPinky4,
    kProfileJointRightHandThumb1,
    kProfileJointRightHandThumb2,
    kProfileJointRightHandThumb3,
    kProfileJointRightHandThumb4,
    kProfileJointRightHandIndex1,
    kProfileJointRightHandIndex2,
    kProfileJointRightHandIndex3,
    kProfileJointRightHandIndex4,
    kProfileJointRightHandMiddle1,
    kProfileJointRightHandMiddle2,
    kProfileJointRightHandMiddle3,
    kProfileJointRightHandMiddle4,
    kProfileJointRightHandRing1,
    kProfileJointRightHandRing2,
    kProfileJointRightHandRing3,
    kProfileJointRightHandRing4,
    kProfileJointRightHandPinky1,
    kProfileJointRightHandPinky2,
    kProfileJointRightHandPinky3,
    kProfileJointRightHandPinky4,
};

struct ProfileSkeletonJoint {
    const char* name = "";
    int parentIndex = -1;
    Vec3 positionMeters{};
    Vec3 sideHint{};
};

using ProfileSkeletonJoints = std::array<ProfileSkeletonJoint, kProfileSkeletonJointCount>;

ProfileSkeletonJoints buildProfileSkeletonJoints(const BodyProfile& profile);
bool isProfileFingerJoint(int jointIndex) noexcept;
bool isProfileHandOrFingerJoint(int jointIndex) noexcept;
int profileWristJoint(ProfileSkeletonHandSide side) noexcept;
int profileHandRootJoint(ProfileSkeletonHandSide side) noexcept;
int profileFingerJoint(
    ProfileSkeletonHandSide side,
    ProfileSkeletonFinger finger,
    int segmentOneBased
) noexcept;

} // namespace ovtr::win32
