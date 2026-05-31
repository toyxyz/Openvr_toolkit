#include "platform/win32/ProfileSkeleton.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

constexpr float kCmToMeters = 0.01f;
constexpr float kSpine0Ratio = 18.0f / 53.0f;
constexpr float kSpine1Ratio = 17.0f / 53.0f;

enum MeasurementIndex {
    FloorToPelvis,
    PelvisWidth,
    PelvisToNeck,
    NeckToHeadTop,
    NeckLength,
    ShoulderWidth,
    ShoulderToElbow,
    ElbowToWrist,
    WristToFinger,
    HipToKnee,
    KneeToAnkle,
    AnkleToToe,
    AnkleHeight,
    ToeTipHeight,
};

constexpr int kFingerJointCountPerHand = 20;
constexpr std::array<float, 5> kFingerBaseXRatios{0.09f, 0.24f, 0.25f, 0.23f, 0.20f};
constexpr std::array<float, 5> kFingerTipXRatios{0.65f, 0.92f, 1.0f, 0.95f, 0.78f};
constexpr std::array<float, 5> kFingerBaseZRatios{0.18f, 0.13f, 0.0f, -0.10f, -0.20f};
constexpr std::array<float, 5> kFingerTipZRatios{0.42f, 0.13f, 0.0f, -0.10f, -0.20f};
constexpr std::array<float, kProfileFingerJointSegments> kFingerSegmentRatios{0.0f, 0.38f, 0.68f, 1.0f};

Vec3 meters(const float xCm, const float yCm, const float zCm) noexcept
{
    return Vec3{xCm * kCmToMeters, yCm * kCmToMeters, zCm * kCmToMeters};
}

float measurement(const BodyProfile& profile, const MeasurementIndex index) noexcept
{
    return profile.measurements[static_cast<std::size_t>(index)];
}

void setJoint(
    ProfileSkeletonJoints& joints,
    const int index,
    const char* name,
    const int parent,
    const Vec3 position,
    const Vec3 sideHint = {}
) {
    joints[static_cast<std::size_t>(index)] = ProfileSkeletonJoint{name, parent, position, sideHint};
}

int firstFingerJoint(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left
        ? kProfileJointLeftHandThumb1
        : kProfileJointRightHandThumb1;
}

const char* fingerJointName(
    const ProfileSkeletonHandSide side,
    const ProfileSkeletonFinger finger,
    const int segment
) noexcept {
    static constexpr std::array<std::array<const char*, kProfileFingerJointSegments>, 5> kLeftNames{{
        {{"LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandThumb4"}},
        {{"LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandIndex4"}},
        {{"LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandMiddle4"}},
        {{"LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandRing4"}},
        {{"LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "LeftHandPinky4"}},
    }};
    static constexpr std::array<std::array<const char*, kProfileFingerJointSegments>, 5> kRightNames{{
        {{"RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandThumb4"}},
        {{"RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandIndex4"}},
        {{"RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandMiddle4"}},
        {{"RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandRing4"}},
        {{"RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "RightHandPinky4"}},
    }};
    const auto& names = side == ProfileSkeletonHandSide::Left ? kLeftNames : kRightNames;
    return names[static_cast<std::size_t>(finger)][static_cast<std::size_t>(segment)];
}

void addFingerChain(
    ProfileSkeletonJoints& joints,
    const ProfileSkeletonHandSide side,
    const ProfileSkeletonFinger finger,
    const float wristX,
    const float wristY,
    const float sideSign,
    const float wristToMiddleTip
) {
    const int fingerOrdinal = static_cast<int>(finger);
    int parent = profileHandRootJoint(side);
    for (int segment = 0; segment < kProfileFingerJointSegments; ++segment) {
        const float t = kFingerSegmentRatios[static_cast<std::size_t>(segment)];
        const float baseX = wristToMiddleTip * kFingerBaseXRatios[static_cast<std::size_t>(fingerOrdinal)];
        const float tipX = wristToMiddleTip * kFingerTipXRatios[static_cast<std::size_t>(fingerOrdinal)];
        const float baseZ = wristToMiddleTip * kFingerBaseZRatios[static_cast<std::size_t>(fingerOrdinal)];
        const float tipZ = wristToMiddleTip * kFingerTipZRatios[static_cast<std::size_t>(fingerOrdinal)];
        const float along = baseX + (tipX - baseX) * t;
        const float spread = baseZ + (tipZ - baseZ) * t;
        const int joint = profileFingerJoint(side, finger, segment + 1);
        setJoint(
            joints,
            joint,
            fingerJointName(side, finger, segment),
            parent,
            meters(wristX + sideSign * along, wristY, spread),
            Vec3{0.0f, 1.0f, 0.0f}
        );
        parent = joint;
    }
}

} // namespace

ProfileSkeletonJoints buildProfileSkeletonJoints(const BodyProfile& profile)
{
    const float pelvisY = measurement(profile, FloorToPelvis);
    const float pelvisHalf = measurement(profile, PelvisWidth) * 0.5f;
    const float pelvisToNeck = measurement(profile, PelvisToNeck);
    const float neckToHeadTop = measurement(profile, NeckToHeadTop);
    const float neckLength = measurement(profile, NeckLength);
    const float shoulderHalf = measurement(profile, ShoulderWidth) * 0.5f;
    const float shoulderToElbow = measurement(profile, ShoulderToElbow);
    const float elbowToWrist = measurement(profile, ElbowToWrist);
    const float wristToFinger = measurement(profile, WristToFinger);
    const float hipToKnee = measurement(profile, HipToKnee);
    const float kneeToAnkle = measurement(profile, KneeToAnkle);
    const float ankleToToe = measurement(profile, AnkleToToe);
    const float ankleY = measurement(profile, AnkleHeight);
    const float toeY = measurement(profile, ToeTipHeight);

    const float spineY = pelvisY + pelvisToNeck * kSpine0Ratio;
    const float spine1Y = spineY + pelvisToNeck * kSpine1Ratio;
    const float spine2Y = pelvisY + pelvisToNeck;
    const float neckY = spine2Y + neckLength;
    const float headTopY = spine2Y + neckToHeadTop;
    const float headY = (neckY + headTopY) * 0.5f;
    const float lowerLegTotal = hipToKnee + kneeToAnkle;
    const float hipToKneeShare = lowerLegTotal > 0.0f ? hipToKnee / lowerLegTotal : 0.0f;
    const float kneeY = pelvisY - (pelvisY - ankleY) * hipToKneeShare;
    const float leftSign = 1.0f;
    const float rightSign = -1.0f;
    const float leftWristX = leftSign * (shoulderHalf + shoulderToElbow + elbowToWrist);
    const float rightWristX = rightSign * (shoulderHalf + shoulderToElbow + elbowToWrist);

    ProfileSkeletonJoints joints{};
    setJoint(joints, kProfileJointHips, "Hips", -1, meters(0.0f, pelvisY, 0.0f));
    setJoint(joints, kProfileJointSpine, "Spine", kProfileJointHips, meters(0.0f, spineY, 0.0f));
    setJoint(joints, kProfileJointSpine1, "Spine1", kProfileJointSpine, meters(0.0f, spine1Y, 0.0f));
    setJoint(joints, kProfileJointSpine2, "Spine2", kProfileJointSpine1, meters(0.0f, spine2Y, 0.0f));
    setJoint(joints, kProfileJointNeck, "Neck", kProfileJointSpine2, meters(0.0f, neckY, 0.0f));
    setJoint(joints, kProfileJointHead, "Head", kProfileJointNeck, meters(0.0f, headY, 0.0f));
    setJoint(joints, kProfileJointHeadTopEnd, "HeadTop_End", kProfileJointHead, meters(0.0f, headTopY, 0.0f));
    setJoint(joints, kProfileJointLeftShoulder, "LeftShoulder", kProfileJointSpine2, meters(leftSign * shoulderHalf, spine2Y, 0.0f));
    setJoint(joints, kProfileJointLeftArm, "LeftArm", kProfileJointLeftShoulder, meters(leftSign * (shoulderHalf + shoulderToElbow), spine2Y, 0.0f));
    setJoint(joints, kProfileJointLeftForeArm, "LeftForeArm", kProfileJointLeftArm, meters(leftWristX, spine2Y, 0.0f));
    setJoint(joints, kProfileJointLeftHand, "LeftHand", kProfileJointLeftForeArm, meters(leftWristX, spine2Y, 0.0f), Vec3{0.0f, 1.0f, 0.0f});
    setJoint(joints, kProfileJointRightShoulder, "RightShoulder", kProfileJointSpine2, meters(rightSign * shoulderHalf, spine2Y, 0.0f));
    setJoint(joints, kProfileJointRightArm, "RightArm", kProfileJointRightShoulder, meters(rightSign * (shoulderHalf + shoulderToElbow), spine2Y, 0.0f));
    setJoint(joints, kProfileJointRightForeArm, "RightForeArm", kProfileJointRightArm, meters(rightWristX, spine2Y, 0.0f));
    setJoint(joints, kProfileJointRightHand, "RightHand", kProfileJointRightForeArm, meters(rightWristX, spine2Y, 0.0f), Vec3{0.0f, 1.0f, 0.0f});
    setJoint(joints, kProfileJointLeftUpLeg, "LeftUpLeg", kProfileJointHips, meters(leftSign * pelvisHalf, pelvisY, 0.0f));
    setJoint(joints, kProfileJointLeftLeg, "LeftLeg", kProfileJointLeftUpLeg, meters(leftSign * pelvisHalf, kneeY, 0.0f));
    setJoint(joints, kProfileJointLeftFoot, "LeftFoot", kProfileJointLeftLeg, meters(leftSign * pelvisHalf, ankleY, 0.0f));
    setJoint(joints, kProfileJointLeftToeBase, "LeftToeBase", kProfileJointLeftFoot, meters(leftSign * pelvisHalf, toeY, ankleToToe));
    setJoint(joints, kProfileJointRightUpLeg, "RightUpLeg", kProfileJointHips, meters(rightSign * pelvisHalf, pelvisY, 0.0f));
    setJoint(joints, kProfileJointRightLeg, "RightLeg", kProfileJointRightUpLeg, meters(rightSign * pelvisHalf, kneeY, 0.0f));
    setJoint(joints, kProfileJointRightFoot, "RightFoot", kProfileJointRightLeg, meters(rightSign * pelvisHalf, ankleY, 0.0f));
    setJoint(joints, kProfileJointRightToeBase, "RightToeBase", kProfileJointRightFoot, meters(rightSign * pelvisHalf, toeY, ankleToToe));

    for (const ProfileSkeletonFinger finger : {
        ProfileSkeletonFinger::Thumb,
        ProfileSkeletonFinger::Index,
        ProfileSkeletonFinger::Middle,
        ProfileSkeletonFinger::Ring,
        ProfileSkeletonFinger::Pinky,
    }) {
        addFingerChain(joints, ProfileSkeletonHandSide::Left, finger, leftWristX, spine2Y, leftSign, wristToFinger);
        addFingerChain(joints, ProfileSkeletonHandSide::Right, finger, rightWristX, spine2Y, rightSign, wristToFinger);
    }
    return joints;
}

bool isProfileFingerJoint(const int jointIndex) noexcept
{
    return (jointIndex >= kProfileJointLeftHandThumb1 && jointIndex <= kProfileJointLeftHandPinky4) ||
        (jointIndex >= kProfileJointRightHandThumb1 && jointIndex <= kProfileJointRightHandPinky4);
}

bool isProfileHandOrFingerJoint(const int jointIndex) noexcept
{
    return jointIndex == kProfileJointLeftHand ||
        jointIndex == kProfileJointRightHand ||
        isProfileFingerJoint(jointIndex);
}

int profileWristJoint(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? kProfileJointLeftForeArm : kProfileJointRightForeArm;
}

int profileHandRootJoint(const ProfileSkeletonHandSide side) noexcept
{
    return side == ProfileSkeletonHandSide::Left ? kProfileJointLeftHand : kProfileJointRightHand;
}

int profileFingerJoint(
    const ProfileSkeletonHandSide side,
    const ProfileSkeletonFinger finger,
    const int segmentOneBased
) noexcept {
    const int clampedSegment = segmentOneBased < 1
        ? 1
        : (segmentOneBased > kProfileFingerJointSegments ? kProfileFingerJointSegments : segmentOneBased);
    return firstFingerJoint(side) +
        static_cast<int>(finger) * kProfileFingerJointSegments +
        (clampedSegment - 1);
}

} // namespace ovtr::win32
