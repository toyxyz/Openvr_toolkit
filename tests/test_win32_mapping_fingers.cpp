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
    requireNear(actual.x, expected.x, message);
    requireNear(actual.y, expected.y, message);
    requireNear(actual.z, expected.z, message);
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
    if (!updateCalibratedMappingActorJoints(actor, skeletalPoses, false, {}, {})) {
        throw std::runtime_error("skeletal finger live solve should update");
    }

    const Vec3 wrist = actor.liveJoints[kProfileJointLeftForeArm].positionMeters;
    requireVecNear(actor.liveJoints[kProfileJointLeftHand].positionMeters, wrist, "left hand root follows wrist");
    requireNear(actor.liveJoints[kProfileJointLeftHandMiddle4].positionMeters.x, wrist.x + 0.19f, "middle tip profile x");
    requireNear(actor.liveJoints[kProfileJointLeftHandMiddle4].positionMeters.z, wrist.z, "middle tip rest spread");
    requireVecNear(actor.liveJoints[kProfileJointLeftHandMiddle4].sideHint, {0.0f, 1.0f, 0.0f}, "finger side hint uses hand tracker");

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
    putSourceBone(transforms, valid, 6, {0.02f, 0.0f, 0.02f});
    putSourceBone(transforms, valid, 11, {0.0f, 0.0f, 0.02f});
    putSourceBone(transforms, valid, 15, {0.0f, 0.0f, 0.10f});
    putSourceBone(transforms, valid, 21, {-0.02f, 0.0f, 0.02f});
    const ProfileSkeletonJoints restForBasis = buildProfileSkeletonJoints(actor.profile);
    const Vec3 preferredRightPalmSide =
        preferredPalmSideForHand(restForBasis, ProfileSkeletonHandSide::Right, kProfileJointRightHand);
    if (preferredRightPalmSide.y >= 0.0f) {
        throw std::runtime_error("right preferred palm side should point downward");
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
    if (directRightBasis.side.y >= 0.0f) {
        throw std::runtime_error("direct right basis should point downward");
    }
    const MappingFingerAlignment rightAlignment =
        makeMappingFingerAlignment(transforms, valid, restForBasis, ProfileSkeletonHandSide::Right);
    if (!rightAlignment.valid) {
        throw std::runtime_error("right hand basis should be valid");
    }
    if (rightAlignment.restBasis.side.y >= 0.0f) {
        throw std::runtime_error(
            std::string("right rest basis should point downward: ") +
            std::to_string(rightAlignment.restBasis.side.y)
        );
    }
    if (rightAlignment.sourceBasis.side.y >= 0.0f) {
        throw std::runtime_error("right source basis should point downward");
    }
    const Vec3 rightPalmDownOffset = alignSkeletalHandPoint(rightAlignment, {0.0f, -0.02f, 0.10f});
    if (rightPalmDownOffset.y >= 0.0f) {
        throw std::runtime_error(
            std::string("right hand basis should preserve palm-down handedness: ") +
            std::to_string(rightPalmDownOffset.y)
        );
    }
}

} // namespace ovtr::test
#endif
