#include "platform/win32/VmcStreamingPose.h"

#include "platform/win32/StreamingOutputTarget.h"
#include "platform/win32/VmcLegRotationContinuity.h"
#include "platform/win32/VmcLimbSpacing.h"
#include "platform/win32/VmcOscWriter.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

struct BoneMap {
    int joint = 0;
    const char* vmcName = "";
};

constexpr BoneMap kBodyBones[] = {
    {kProfileJointHips, "Hips"},
    {kProfileJointSpine, "Spine"},
    {kProfileJointSpine1, "Chest"},
    {kProfileJointSpine2, "UpperChest"},
    {kProfileJointNeck, "Neck"},
    {kProfileJointHead, "Head"},
    {kProfileJointLeftShoulder, "LeftShoulder"},
    {kProfileJointLeftArm, "LeftUpperArm"},
    {kProfileJointLeftForeArm, "LeftLowerArm"},
    {kProfileJointLeftHand, "LeftHand"},
    {kProfileJointRightShoulder, "RightShoulder"},
    {kProfileJointRightArm, "RightUpperArm"},
    {kProfileJointRightForeArm, "RightLowerArm"},
    {kProfileJointRightHand, "RightHand"},
    {kProfileJointLeftUpLeg, "LeftUpperLeg"},
    {kProfileJointLeftLeg, "LeftLowerLeg"},
    {kProfileJointLeftFoot, "LeftFoot"},
    {kProfileJointRightUpLeg, "RightUpperLeg"},
    {kProfileJointRightLeg, "RightLowerLeg"},
    {kProfileJointRightFoot, "RightFoot"},
};

constexpr const char* kFingerNames[2][5][3] = {
    {
        {"LeftThumbProximal", "LeftThumbIntermediate", "LeftThumbDistal"},
        {"LeftIndexProximal", "LeftIndexIntermediate", "LeftIndexDistal"},
        {"LeftMiddleProximal", "LeftMiddleIntermediate", "LeftMiddleDistal"},
        {"LeftRingProximal", "LeftRingIntermediate", "LeftRingDistal"},
        {"LeftLittleProximal", "LeftLittleIntermediate", "LeftLittleDistal"},
    },
    {
        {"RightThumbProximal", "RightThumbIntermediate", "RightThumbDistal"},
        {"RightIndexProximal", "RightIndexIntermediate", "RightIndexDistal"},
        {"RightMiddleProximal", "RightMiddleIntermediate", "RightMiddleDistal"},
        {"RightRingProximal", "RightRingIntermediate", "RightRingDistal"},
        {"RightLittleProximal", "RightLittleIntermediate", "RightLittleDistal"},
    },
};

float currentTimeSeconds() noexcept
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<float>(now).count();
}

VmcOscTransform transformForBone(const SkeletonBonePose& bone) noexcept
{
    return VmcOscTransform{
        appPositionToVmcPosition(bone.localTranslationMeters),
        appRotationToVmcRotation(bone.localRotation)
    };
}

VmcOscTransform transformForBone(
    const SkeletonBonePose& bone,
    const std::array<float, 4>& localRotation
) noexcept {
    return VmcOscTransform{
        appPositionToVmcPosition(bone.localTranslationMeters),
        appRotationToVmcRotation(localRotation)
    };
}

void addJointIfValid(
    VmcOscWriter& writer,
    const SkeletonPose& pose,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations,
    const int joint,
    const char* vmcName
)
{
    const SkeletonBonePose& bone = pose.bones[static_cast<std::size_t>(joint)];
    if (bone.valid) {
        writer.addBoneTransform(vmcName, transformForBone(bone, localRotations[static_cast<std::size_t>(joint)]));
    }
}

void addFingerBones(
    VmcOscWriter& writer,
    const SkeletonPose& pose,
    const std::array<std::array<float, 4>, kProfileSkeletonJointCount>& localRotations
)
{
    for (int side = 0; side < 2; ++side) {
        const auto handSide = side == 0 ? ProfileSkeletonHandSide::Left : ProfileSkeletonHandSide::Right;
        for (int finger = 0; finger < 5; ++finger) {
            const auto fingerId = static_cast<ProfileSkeletonFinger>(finger);
            for (int segment = 0; segment < 3; ++segment) {
                addJointIfValid(
                    writer,
                    pose,
                    localRotations,
                    profileFingerJoint(handSide, fingerId, segment + 1),
                    kFingerNames[side][finger][segment]
                );
            }
        }
    }
}

std::vector<std::vector<std::uint8_t>> makeVmcPackets(
    const MappingActor& actor,
    VmcLegRotationContinuity& continuity,
    const float armSpacingDegrees,
    const float legSpacingDegrees
)
{
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    const ProfileSkeletonJoints poseJoints = actor.liveJointsValid
        ? actor.liveJoints
        : computeSkeletonPoseWorldJoints(rest, actor.liveSkeletonPose);
    auto localRotations =
        makeVmcLocalRotationsWithContinuity(rest, poseJoints, actor.liveSkeletonPose, continuity);
    applyVmcLimbSpacing(localRotations, armSpacingDegrees, legSpacingDegrees);
    VmcOscWriter writer;
    writer.addStatus(1);
    writer.addTime(currentTimeSeconds());
    writer.addRootTransform("root", VmcOscTransform{});
    for (const BoneMap& bone : kBodyBones) {
        addJointIfValid(writer, actor.liveSkeletonPose, localRotations, bone.joint, bone.vmcName);
    }
    addFingerBones(writer, actor.liveSkeletonPose, localRotations);
    return {writer.makeBundle()};
}

} // namespace

std::array<float, 3> appPositionToVmcPosition(const Vec3 position) noexcept
{
    return {-position.x, position.y, position.z};
}

std::array<float, 4> appRotationToVmcRotation(const std::array<float, 4>& rotation) noexcept
{
    return {rotation[0], -rotation[1], -rotation[2], rotation[3]};
}

bool sendMappingActorVmcPose(
    AppStreamingState& streamingState,
    const MappingActor& actor,
    std::string& error
) {
    if (streamingState.streamingOutputTarget != StreamingOutputTarget::Vmc) {
        stopVmcStreamingOutput(streamingState);
        return true;
    }
    if (!actor.liveJointsValid) {
        resetVmcLegRotationContinuity(streamingState.vmcLegRotationContinuity);
        return true;
    }
    if (!streamingState.vmcSender.configure(
            true,
            streamingState.vmcSendHost,
            streamingState.vmcSendPort,
            error
        )) {
        return false;
    }
    const std::vector<std::vector<std::uint8_t>> packets =
        makeVmcPackets(
            actor,
            streamingState.vmcLegRotationContinuity,
            streamingState.vmcArmSpacingDegrees,
            streamingState.vmcLegSpacingDegrees
        );
    for (const std::vector<std::uint8_t>& packet : packets) {
        if (!streamingState.vmcSender.sendPacket(packet, error)) {
            return false;
        }
    }
    return true;
}

void stopVmcStreamingOutput(AppStreamingState& streamingState) noexcept
{
    streamingState.vmcSender.stop();
    resetVmcLegRotationContinuity(streamingState.vmcLegRotationContinuity);
}

} // namespace ovtr::win32
