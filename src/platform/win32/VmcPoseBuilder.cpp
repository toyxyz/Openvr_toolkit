#include "platform/win32/VmcPoseBuilder.h"

#include "data/VmcSyntheticPose.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppState.h"
#include "platform/win32/VmcFingerDisplayRotation.h"
#include "platform/win32/VmcFingerRotationFilter.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/VmcFingerState.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>

namespace ovtr::win32 {

std::array<float, 3> vmcLocalPositionToAppOffset(const std::array<float, 3>& value) noexcept
{
    return {-value[0], value[1], value[2]};
}

std::array<float, 4> vmcLocalRotationToAppRotation(const std::array<float, 4>& value) noexcept
{
    return {value[0], -value[1], -value[2], value[3]};
}

namespace {

struct PoseAnchor {
    std::array<float, 3> position{};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

const ovtr::PoseSample* validPoseForRuntime(const ovtr::PosePollResult& poses, const std::uint32_t runtimeIndex)
{
    if (runtimeIndex == kNoSelectedRuntimeIndex) {
        return nullptr;
    }
    for (const ovtr::PoseSample& pose : poses.poses) {
        if (pose.runtimeIndex == runtimeIndex && (pose.flags & ovtr::PoseFlagPoseValid) != 0) {
            return &pose;
        }
    }
    return nullptr;
}

PoseAnchor anchorForSide(const AppWindowState& state, const ovtr::PosePollResult& poses, const ovtr::SkeletalHandSide side)
{
    const MappingTrackerRole role = side == ovtr::SkeletalHandSide::Left
        ? MappingTrackerRole::LeftHand
        : MappingTrackerRole::RightHand;
    const std::uint32_t runtimeIndex =
        state.mappingDeviceRuntimeIndices[static_cast<std::size_t>(mappingSlotForRole(role))];
    if (const ovtr::PoseSample* pose = validPoseForRuntime(poses, runtimeIndex)) {
        return {pose->position, pose->rotation};
    }
    return {{{side == ovtr::SkeletalHandSide::Left ? -0.12f : 0.12f, 0.0f, 0.0f}}, {0.0f, 0.0f, 0.0f, 1.0f}};
}

bool handHasAnyBone(const VmcFingerSideState& hand) noexcept
{
    for (const bool valid : hand.valid) {
        if (valid) {
            return true;
        }
    }
    return false;
}

void appendPose(
    ovtr::PosePollResult& poses,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t boneIndex,
    const std::array<float, 3>& position,
    const std::array<float, 4>& rotation
) {
    ovtr::PoseSample pose;
    pose.deviceId = ovtr::vmcFingerRuntimeIndex(side, boneIndex);
    pose.runtimeIndex = pose.deviceId;
    pose.position = position;
    pose.rotation = rotation;
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid | ovtr::PoseFlagRecordEnabled;
    poses.poses.push_back(pose);
}

std::array<float, 3> addArray3(const std::array<float, 3>& left, const std::array<float, 3>& right) noexcept
{
    return {left[0] + right[0], left[1] + right[1], left[2] + right[2]};
}

std::array<float, 3> scaleArray3(const std::array<float, 3>& value, const float scale) noexcept
{
    return {value[0] * scale, value[1] * scale, value[2] * scale};
}

float lengthArray3(const std::array<float, 3>& value) noexcept
{
    return std::sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
}

bool usableOffset(const std::array<float, 3>& value) noexcept
{
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]) &&
        (std::fabs(value[0]) + std::fabs(value[1]) + std::fabs(value[2])) > 0.000001f;
}

std::array<float, 3> fallbackBoneOffset(
    const ovtr::SkeletalHandSide side,
    const std::uint32_t finger,
    const std::uint32_t segment
) noexcept {
    static constexpr std::array<float, 5> kSpread = {0.045f, 0.025f, 0.0f, -0.020f, -0.040f};
    static constexpr std::array<float, 5> kForward = {0.018f, 0.036f, 0.040f, 0.036f, 0.028f};
    static constexpr std::array<std::array<float, 4>, 5> kLengths = {{
        {0.030f, 0.025f, 0.020f, 0.016f},
        {0.035f, 0.030f, 0.024f, 0.018f},
        {0.038f, 0.032f, 0.026f, 0.020f},
        {0.034f, 0.029f, 0.023f, 0.017f},
        {0.028f, 0.024f, 0.019f, 0.015f},
    }};
    if (segment == 0) {
        const float mirror = side == ovtr::SkeletalHandSide::Left ? -1.0f : 1.0f;
        return {mirror * kSpread[finger], 0.0f, kForward[finger]};
    }
    return {0.0f, 0.0f, kLengths[finger][segment - 1u]};
}

std::array<float, 3> localBoneOffset(
    const VmcFingerSideState& hand,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t finger,
    const std::uint32_t segment
) noexcept {
    const std::uint32_t nameBone = finger * 3u + segment;
    if (segment < 3u && hand.valid[nameBone] && usableOffset(hand.positions[nameBone])) {
        return vmcLocalPositionToAppOffset(hand.positions[nameBone]);
    }
    if (segment == 3u) {
        const std::uint32_t distalBone = finger * 3u + 2u;
        if (hand.valid[distalBone] && usableOffset(hand.positions[distalBone])) {
            const std::array<float, 3> fallbackTip = fallbackBoneOffset(side, finger, segment);
            const float sourceLength = lengthArray3(hand.positions[distalBone]);
            const float tipLength = lengthArray3(fallbackTip);
            if (sourceLength > 0.000001f && tipLength > 0.000001f) {
                return scaleArray3(
                    vmcLocalPositionToAppOffset(hand.positions[distalBone]),
                    tipLength / sourceLength
                );
            }
        }
    }
    return fallbackBoneOffset(side, finger, segment);
}

void appendHand(
    ovtr::PosePollResult& poses,
    const VmcFingerSideState& hand,
    const PoseAnchor& anchor,
    const ovtr::SkeletalHandSide side
) {
    appendPose(poses, side, 0, anchor.position, anchor.rotation);
    const std::array<float, 3> palmSide = ovtr::rotatePositionByQuaternion(anchor.rotation, {0.0f, 1.0f, 0.0f});
    for (std::uint32_t finger = 0; finger < 5u; ++finger) {
        std::array<float, 3> parentPosition = anchor.position;
        std::array<float, 4> parentRotation = anchor.rotation;
        std::array<std::array<float, 3>, 4> points{};
        for (std::uint32_t segment = 0; segment < 3u; ++segment) {
            const std::uint32_t nameBone = finger * 3u + segment;
            const std::array<float, 3> localOffset = localBoneOffset(hand, side, finger, segment);
            const std::array<float, 3> bonePosition = addArray3(
                parentPosition,
                ovtr::rotatePositionByQuaternion(parentRotation, localOffset)
            );
            const std::array<float, 4> localRotation = hand.valid[nameBone]
                ? vmcLocalRotationToAppRotation(hand.rotations[nameBone])
                : std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
            const std::array<float, 4> filteredLocalRotation =
                removeVmcFingerLocalRoll(localRotation, localBoneOffset(hand, side, finger, segment + 1u));
            const std::array<float, 4> boneRotation = ovtr::multiplyQuaternion(parentRotation, filteredLocalRotation);
            points[segment] = bonePosition;
            parentPosition = bonePosition;
            parentRotation = boneRotation;
        }
        points[3] = addArray3(
            parentPosition,
            ovtr::rotatePositionByQuaternion(parentRotation, localBoneOffset(hand, side, finger, 3u))
        );
        for (std::uint32_t segment = 0; segment < 4u; ++segment) {
            const std::uint32_t start = segment < 3u ? segment : 2u;
            const std::uint32_t end = segment < 3u ? segment + 1u : 3u;
            appendPose(
                poses,
                side,
                1u + finger * 4u + segment,
                points[segment],
                vmcFingerDisplayRotationForSegment(points[start], points[end], palmSide)
            );
        }
    }
}

} // namespace

void appendVmcFingerPoses(AppWindowState& state, ovtr::PosePollResult& poses)
{
    if (!state.vmcReceiveEnabled) {
        return;
    }
    const VmcFingerSnapshot snapshot = state.vmcReceiver.snapshot();
    const auto now = std::chrono::steady_clock::now();
    for (const ovtr::SkeletalHandSide side : {ovtr::SkeletalHandSide::Left, ovtr::SkeletalHandSide::Right}) {
        const VmcFingerSideState& hand = snapshot.hands[static_cast<std::size_t>(vmcSideIndex(side))];
        if (!isVmcFingerSideFresh(hand, now) || !handHasAnyBone(hand)) {
            continue;
        }
        appendHand(poses, hand, anchorForSide(state, poses, side), side);
    }
}

} // namespace ovtr::win32
