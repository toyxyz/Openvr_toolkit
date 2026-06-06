#include "TestCases.h"

#ifdef _WIN32
#include "data/SkeletalSyntheticPose.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingHandBasis.h"
#include "math/PoseTransform.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

void requireNear(const float actual, const float expected, const char* message)
{
    if (std::fabs(actual - expected) > 0.001f) {
        throw std::runtime_error(message);
    }
}

void requireVecNear(const ovtr::win32::Vec3 actual, const ovtr::win32::Vec3 expected, const char* message)
{
    if (std::fabs(actual.x - expected.x) > 0.001f ||
        std::fabs(actual.y - expected.y) > 0.001f ||
        std::fabs(actual.z - expected.z) > 0.001f) {
        throw std::runtime_error(
            std::string(message) + ": " +
            std::to_string(actual.x) + ", " +
            std::to_string(actual.y) + ", " +
            std::to_string(actual.z)
        );
    }
}

std::array<std::uint32_t, ovtr::win32::kMappingSlotCount> sequentialMapping()
{
    std::array<std::uint32_t, ovtr::win32::kMappingSlotCount> mapping{};
    for (std::size_t index = 0; index < mapping.size(); ++index) {
        mapping[index] = static_cast<std::uint32_t>(index + 1u);
    }
    return mapping;
}

ovtr::PosePollResult posesForTargets(
    const std::array<ovtr::win32::MappingTransform, ovtr::win32::kMappingSlotCount>& targets,
    const std::array<std::uint32_t, ovtr::win32::kMappingSlotCount>& mapping
) {
    ovtr::PosePollResult result;
    for (std::size_t index = 0; index < targets.size(); ++index) {
        ovtr::PoseSample pose;
        pose.runtimeIndex = mapping[index];
        pose.position = {targets[index].position.x, targets[index].position.y, targets[index].position.z};
        pose.rotation = targets[index].rotation;
        pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;
        result.poses.push_back(pose);
    }
    return result;
}

void appendSkeletalPose(
    ovtr::PosePollResult& result,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t boneIndex,
    const ovtr::win32::Vec3 position,
    const std::array<float, 4> rotation = {0.0f, 0.0f, 0.0f, 1.0f}
) {
    ovtr::PoseSample pose;
    pose.runtimeIndex = ovtr::skeletalBoneRuntimeIndex(side, boneIndex);
    pose.deviceId = pose.runtimeIndex;
    pose.position = {position.x, position.y, position.z};
    pose.rotation = rotation;
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;
    result.poses.push_back(pose);
}

void appendSourceFinger(
    ovtr::PosePollResult& result,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t firstBone,
    const std::uint32_t count,
    const float spreadX,
    const float tipZ
) {
    for (std::uint32_t index = 0; index < count; ++index) {
        const float t = static_cast<float>(index + 1u) / static_cast<float>(count);
        appendSkeletalPose(result, side, firstBone + index, {spreadX, 0.0f, tipZ * t});
    }
}

void appendStraightSkeletalHand(ovtr::PosePollResult& result, const ovtr::SkeletalHandSide side)
{
    appendSkeletalPose(result, side, 0, {0.0f, 0.0f, 0.0f});
    appendSkeletalPose(result, side, 1, {0.0f, 0.0f, 0.0f}, ovtr::quaternionFromEulerDegrees({90.0f, 0.0f, 90.0f}));
    appendSourceFinger(result, side, 2, 4, 0.045f, 0.070f);
    appendSourceFinger(result, side, 6, 5, 0.025f, 0.095f);
    appendSourceFinger(result, side, 11, 5, 0.0f, 0.100f);
    appendSourceFinger(result, side, 16, 5, -0.020f, 0.092f);
    appendSourceFinger(result, side, 21, 5, -0.040f, 0.074f);
}

void appendCurledSourceFinger(
    ovtr::PosePollResult& result,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t firstBone,
    const float spreadX,
    const float tipZ,
    const bool latePinkyCurl
) {
    for (std::uint32_t index = 0; index < 5; ++index) {
        const float t = static_cast<float>(index + 1u) / 5.0f;
        const float curlT = latePinkyCurl && index < 2 ? 0.0f : t;
        appendSkeletalPose(result, side, firstBone + index, {spreadX, -0.085f * curlT, tipZ * t});
    }
}

void appendCurledSkeletalHand(ovtr::PosePollResult& result, const ovtr::SkeletalHandSide side)
{
    appendSkeletalPose(result, side, 0, {0.0f, 0.0f, 0.0f});
    appendSkeletalPose(result, side, 1, {0.0f, 0.0f, 0.0f});
    appendSourceFinger(result, side, 2, 4, 0.045f, 0.070f);
    appendCurledSourceFinger(result, side, 6, 0.025f, 0.095f, false);
    appendCurledSourceFinger(result, side, 11, 0.0f, 0.100f, false);
    appendCurledSourceFinger(result, side, 16, -0.020f, 0.092f, false);
    appendCurledSourceFinger(result, side, 21, -0.040f, 0.074f, true);
}

float fingerSecondSegmentCurlDegrees(const ovtr::win32::ProfileSkeletonJoints& joints, const int joint1, const int joint2)
{
    const ovtr::win32::Vec3 delta = ovtr::win32::subMappingVec3(
        joints[static_cast<std::size_t>(joint2)].positionMeters,
        joints[static_cast<std::size_t>(joint1)].positionMeters
    );
    return std::atan2(std::fabs(delta.y), std::fabs(delta.x)) * 57.2957795f;
}

void putSourceBone(
    std::array<ovtr::win32::MappingTransform, ovtr::kSkeletalHandBoneCount>& transforms,
    std::array<bool, ovtr::kSkeletalHandBoneCount>& valid,
    const std::uint32_t bone,
    const ovtr::win32::Vec3 position
) {
    transforms[bone].position = position;
    transforms[bone].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    valid[bone] = true;
}

} // namespace

namespace ovtr::test {

void testWin32MappingFingerSolve()
{
    using namespace ovtr::win32;

    MappingActor actor;
    const auto mapping = sequentialMapping();
    auto targets = mappingCalibrationRestTargets(actor.profile);
    const MappingCalibrationStatus status = captureMappingActorCalibration(
        actor,
        mapping,
        posesForTargets(targets, mapping),
        false,
        {},
        {}
    );
    if (!status.success) {
        throw std::runtime_error("finger test calibration should succeed");
    }

    targets[0].position.y += 0.10f;
    targets[1].position.y += 0.10f;
    targets[2].position.y += 0.10f;
    ovtr::PosePollResult skeletalPoses = posesForTargets(targets, mapping);
    appendStraightSkeletalHand(skeletalPoses, ovtr::SkeletalHandSide::Left);
    appendStraightSkeletalHand(skeletalPoses, ovtr::SkeletalHandSide::Right);
    if (!updateCalibratedMappingActorJoints(actor, skeletalPoses, false, {}, {})) {
        throw std::runtime_error("skeletal finger live solve should update");
    }

    const Vec3 wrist = actor.liveJoints[kProfileJointLeftForeArm].positionMeters;
    requireVecNear(actor.liveJoints[kProfileJointLeftHand].positionMeters, wrist, "left hand root follows wrist");
    requireNear(actor.liveJoints[kProfileJointLeftHandMiddle1].positionMeters.x, wrist.x + 0.095f, "middle palm segment");
    requireNear(actor.liveJoints[kProfileJointLeftHandMiddle4].positionMeters.x, wrist.x + 0.19f, "middle tip profile x");
    requireNear(actor.liveJoints[kProfileJointLeftHandMiddle4].positionMeters.z, wrist.z, "middle tip rest spread");
    const auto sideAxes = computeSkeletonPoseWorldSideAxes(buildProfileSkeletonJoints(actor.profile), actor.liveSkeletonPose);
    if (sideAxes[kProfileJointLeftHandMiddle1].y <= 0.9f) {
        throw std::runtime_error("finger root pose roll should stay palm-oriented");
    }
    if (sideAxes[kProfileJointLeftHandMiddle4].y <= 0.6f) {
        throw std::runtime_error("finger pose roll should stay palm-oriented");
    }
    if (sideAxes[kProfileJointRightHandMiddle1].y <= 0.9f) {
        throw std::runtime_error("right finger root pose roll should stay palm-oriented");
    }
    if (sideAxes[kProfileJointRightHandMiddle4].y <= 0.6f) {
        throw std::runtime_error("right finger pose roll should stay palm-oriented");
    }

    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(targets, mapping), false, {}, {})) {
        throw std::runtime_error("missing skeletal input should keep body tracking");
    }
    requireNear(
        actor.liveJoints[kProfileJointLeftHandMiddle4].positionMeters.x,
        wrist.x + 0.19f,
        "missing hand keeps fingers"
    );

    std::array<MappingTransform, ovtr::kSkeletalHandBoneCount> transforms{};
    std::array<bool, ovtr::kSkeletalHandBoneCount> valid{};
    putSourceBone(transforms, valid, 0, {0.0f, 0.0f, 0.0f});
    putSourceBone(transforms, valid, 1, {0.0f, 0.0f, 0.0f});
    putSourceBone(transforms, valid, 6, {-0.02f, 0.0f, 0.02f});
    putSourceBone(transforms, valid, 11, {0.0f, 0.0f, 0.02f});
    putSourceBone(transforms, valid, 15, {0.0f, -0.02f, 0.10f});
    putSourceBone(transforms, valid, 21, {0.02f, 0.0f, 0.02f});
    const ProfileSkeletonJoints restForBasis = buildProfileSkeletonJoints(actor.profile);
    const Vec3 preferredRightPalmSide =
        preferredPalmSideForHand(restForBasis, ProfileSkeletonHandSide::Right, kProfileJointRightHand);
    if (preferredRightPalmSide.y <= 0.0f) {
        throw std::runtime_error("right preferred palm side should match profile upward axis");
    }
    if (profileHandRootJoint(ProfileSkeletonHandSide::Right) != kProfileJointRightHand) {
        throw std::runtime_error("right hand root lookup failed");
    }
    const MappingHandBasis directRightBasis = makeHandBasisFromPoints(
        restForBasis[kProfileJointRightHand].positionMeters,
        restForBasis[profileFingerJoint(ProfileSkeletonHandSide::Right, ProfileSkeletonFinger::Middle, 1)].positionMeters,
        restForBasis[profileFingerJoint(ProfileSkeletonHandSide::Right, ProfileSkeletonFinger::Index, 1)].positionMeters,
        restForBasis[profileFingerJoint(ProfileSkeletonHandSide::Right, ProfileSkeletonFinger::Pinky, 1)].positionMeters,
        preferredRightPalmSide
    );
    if (directRightBasis.side.y <= 0.0f) {
        throw std::runtime_error("direct right basis should match profile upward axis");
    }
    const MappingFingerAlignment rightAlignment =
        makeMappingFingerAlignment(transforms, valid, restForBasis, ProfileSkeletonHandSide::Right);
    if (!rightAlignment.valid) {
        throw std::runtime_error("right hand basis should be valid");
    }
    if (rightAlignment.restBasis.side.y <= 0.0f) {
        throw std::runtime_error(
            std::string("right rest basis should match profile upward axis: ") +
            std::to_string(rightAlignment.restBasis.side.y)
        );
    }
    if (rightAlignment.sourceBasis.side.y >= 0.0f) {
        throw std::runtime_error("right source basis should use mirrored palm side");
    }
    const Vec3 rightPalmOffset = alignSkeletalHandPoint(rightAlignment, {0.0f, -0.02f, 0.10f});
    if (rightPalmOffset.y <= 0.0f) {
        throw std::runtime_error(
            std::string("right hand mirrored basis should flip curl across profile palm axis: ") +
            std::to_string(rightPalmOffset.y)
        );
    }
    auto twistedTransforms = transforms;
    twistedTransforms[0].rotation = ovtr::quaternionFromEulerDegrees({0.0f, 0.0f, 135.0f});
    twistedTransforms[1].rotation = ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    const MappingFingerAlignment twistedAlignment =
        makeMappingFingerAlignment(twistedTransforms, valid, restForBasis, ProfileSkeletonHandSide::Right);
    const Vec3 twistedOffset = alignSkeletalHandPoint(twistedAlignment, {0.0f, -0.02f, 0.10f});
    requireVecNear(twistedOffset, rightPalmOffset, "finger alignment ignores raw source bone roll");

    ovtr::PosePollResult curledPoses = posesForTargets(targets, mapping);
    appendCurledSkeletalHand(curledPoses, ovtr::SkeletalHandSide::Left);
    if (!updateCalibratedMappingActorJoints(actor, curledPoses, false, {}, {})) {
        throw std::runtime_error("curled skeletal finger live solve should update");
    }
    const float ringCurl = fingerSecondSegmentCurlDegrees(
        actor.liveJoints,
        kProfileJointLeftHandRing1,
        kProfileJointLeftHandRing2
    );
    const float pinkyCurl = fingerSecondSegmentCurlDegrees(
        actor.liveJoints,
        kProfileJointLeftHandPinky1,
        kProfileJointLeftHandPinky2
    );
    if (ringCurl - pinkyCurl > 20.0f) {
        throw std::runtime_error("pinky second finger segment curl should not lag ring by more than 20 degrees");
    }
}

} // namespace ovtr::test
#endif
