#include "TestCases.h"
#include "TestSupport.h"

#ifdef _WIN32
#include "data/VmcSyntheticPose.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MappingVmcFingerSolve.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/VmcFingerState.h"
#include "platform/win32/VmcOscParser.h"
#include "platform/win32/VmcOscWriter.h"
#include "platform/win32/VmcPoseBuilder.h"
#include "platform/win32/VmcLegRotationContinuity.h"
#include "platform/win32/VmcLimbSpacing.h"
#include "platform/win32/VmcStreamingPose.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

namespace {

void appendPaddedString(std::vector<std::uint8_t>& bytes, const char* text)
{
    const std::size_t start = bytes.size();
    bytes.insert(bytes.end(), text, text + std::strlen(text));
    bytes.push_back(0);
    while ((bytes.size() - start) % 4u != 0u) {
        bytes.push_back(0);
    }
}

void appendBigEndianInt32(std::vector<std::uint8_t>& bytes, const std::int32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    bytes.push_back(static_cast<std::uint8_t>(value & 0xff));
}

void appendBigEndianFloat(std::vector<std::uint8_t>& bytes, const float value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    appendBigEndianInt32(bytes, static_cast<std::int32_t>(bits));
}

std::vector<std::uint8_t> makeBonePosMessage(const char* boneName)
{
    std::vector<std::uint8_t> bytes;
    appendPaddedString(bytes, "/VMC/Ext/Bone/Pos");
    appendPaddedString(bytes, ",sfffffff");
    appendPaddedString(bytes, boneName);
    for (const float value : {1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f}) {
        appendBigEndianFloat(bytes, value);
    }
    return bytes;
}

std::vector<std::uint8_t> makeBundle(const std::vector<std::uint8_t>& message)
{
    std::vector<std::uint8_t> bytes;
    appendPaddedString(bytes, "#bundle");
    for (int index = 0; index < 8; ++index) {
        bytes.push_back(0);
    }
    appendBigEndianInt32(bytes, static_cast<std::int32_t>(message.size()));
    bytes.insert(bytes.end(), message.begin(), message.end());
    return bytes;
}

void appendVmcPose(
    ovtr::PosePollResult& poses,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t boneIndex,
    const ovtr::win32::Vec3 position
) {
    ovtr::PoseSample pose;
    pose.runtimeIndex = ovtr::vmcFingerRuntimeIndex(side, boneIndex);
    pose.deviceId = pose.runtimeIndex;
    pose.position = {position.x, position.y, position.z};
    pose.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;
    poses.poses.push_back(pose);
}

void appendStraightVmcHand(ovtr::PosePollResult& poses, const ovtr::SkeletalHandSide side)
{
    static constexpr float kSpread[5] = {0.045f, 0.025f, 0.0f, -0.020f, -0.040f};
    static constexpr float kLength[5] = {0.070f, 0.095f, 0.100f, 0.092f, 0.074f};
    appendVmcPose(poses, side, 0, {});
    for (std::uint32_t finger = 0; finger < 5u; ++finger) {
        for (std::uint32_t segment = 0; segment < 4u; ++segment) {
            const float t = static_cast<float>(segment + 1u) / 4.0f;
            appendVmcPose(poses, side, 1u + finger * 4u + segment, {kSpread[finger], 0.0f, kLength[finger] * t});
        }
    }
}

float quaternionAbsDot(const std::array<float, 4>& a, const std::array<float, 4>& b)
{
    return std::fabs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
}

} // namespace

namespace ovtr::test {

void testWin32VmcFingerInput()
{
    ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
    std::uint32_t boneIndex = 0;
    const std::uint32_t rightTip = ovtr::vmcFingerRuntimeIndex(ovtr::SkeletalHandSide::Right, 20);
    require(ovtr::decodeVmcFingerRuntimeIndex(rightTip, side, boneIndex), "decode VMC runtime index");
    require(side == ovtr::SkeletalHandSide::Right && boneIndex == 20, "decode VMC side and bone");
    require(!ovtr::isSkeletalBoneRuntimeIndex(rightTip), "VMC runtime range does not collide with SteamVR skeletal");

    std::uint32_t nameBone = 0;
    require(
        ovtr::win32::parseVmcFingerBoneName("LeftIndexIntermediate", side, nameBone),
        "parse VMC left index intermediate"
    );
    require(side == ovtr::SkeletalHandSide::Left && nameBone == 4, "parse VMC left bone index");
    require(!ovtr::win32::parseVmcFingerBoneName("LeftUpperArm", side, nameBone), "ignore non-finger VMC bone");

    const std::vector<std::uint8_t> message = makeBonePosMessage("RightLittleDistal");
    std::vector<ovtr::win32::VmcOscBonePose> parsed;
    require(ovtr::win32::parseVmcOscPacket(message.data(), message.size(), parsed), "parse VMC OSC message");
    require(parsed.size() == 1 && parsed[0].name == "RightLittleDistal", "extract VMC bone message");
    require(parsed[0].position[0] == 1.0f && parsed[0].rotation[3] == 1.0f, "extract VMC transform values");

    parsed.clear();
    const std::vector<std::uint8_t> bundle = makeBundle(message);
    require(ovtr::win32::parseVmcOscPacket(bundle.data(), bundle.size(), parsed), "parse VMC OSC bundle");
    require(parsed.size() == 1 && parsed[0].name == "RightLittleDistal", "extract bundled VMC bone");

    ovtr::win32::VmcOscWriter writer;
    writer.addStatus(1);
    writer.addTime(1.0f);
    writer.addRootTransform("root", {});
    for (int index = 0; index < 24; ++index) {
        writer.addBoneTransform("LeftIndexProximal", {});
    }
    const std::vector<std::uint8_t> largeBundle = writer.makeBundle();
    const std::vector<std::vector<std::uint8_t>> splitBundles = writer.makeBundles(1200);
    require(largeBundle.size() > 1200, "VMC test bundle exceeds split threshold");
    require(splitBundles.size() > 1, "VMC writer splits oversized bundles");
    for (const std::vector<std::uint8_t>& splitBundle : splitBundles) {
        require(!splitBundle.empty() && splitBundle.size() <= 1200, "VMC split bundle stays within limit");
    }

    const std::array<float, 3> appOffset =
        ovtr::win32::vmcLocalPositionToAppOffset({0.03f, 0.01f, 0.02f});
    require(
        appOffset[0] == -0.03f && appOffset[1] == 0.01f && appOffset[2] == 0.02f,
        "VMC local X mirrors into app hand offset convention"
    );
    const std::array<float, 4> appRotation =
        ovtr::win32::vmcLocalRotationToAppRotation({0.1f, 0.2f, 0.3f, 0.9f});
    require(
        appRotation[0] == 0.1f && appRotation[1] == -0.2f &&
            appRotation[2] == -0.3f && appRotation[3] == 0.9f,
        "VMC local rotation mirrors into app hand rotation convention"
    );
    const std::array<float, 3> vmcOffset =
        ovtr::win32::appPositionToVmcPosition({-0.03f, 0.01f, 0.02f});
    require(vmcOffset[0] == 0.03f && vmcOffset[1] == 0.01f, "app position mirrors back to VMC");
    const std::array<float, 4> vmcRotation =
        ovtr::win32::appRotationToVmcRotation(appRotation);
    require(
        vmcRotation[0] == 0.1f && vmcRotation[1] == 0.2f &&
            vmcRotation[2] == 0.3f && vmcRotation[3] == 0.9f,
        "app rotation mirrors back to VMC"
    );

    ovtr::win32::AppWindowState state;
    appendVmcPose(state.poses, ovtr::SkeletalHandSide::Left, 0, {});
    appendVmcPose(state.poses, ovtr::SkeletalHandSide::Right, 0, {});
    const std::vector<ovtr::win32::DeviceListRow> panelRows = ovtr::win32::makeDevicePanelRows(state);
    require(panelRows.size() == 2, "device panel includes VMC finger rows");
    require(panelRows[0].customName == L"VMC Finger Left", "left VMC row label");
    require(panelRows[1].customName == L"VMC Finger Right", "right VMC row label");
    require(ovtr::win32::makeFingerInputRows(state, 0).size() == 1, "left finger dropdown lists left VMC only");
    require(ovtr::win32::makeFingerInputRows(state, 1).size() == 1, "right finger dropdown lists right VMC only");

    ovtr::win32::MappingActor actor;
    actor.mappingFingerRuntimeIndices[0] = ovtr::vmcFingerRuntimeIndex(ovtr::SkeletalHandSide::Left, 0);
    require(
        ovtr::win32::mappingVmcFingerInputEnabledForSide(actor, ovtr::win32::ProfileSkeletonHandSide::Left),
        "left VMC source enables left mapping solve"
    );

    ovtr::PosePollResult poses;
    appendStraightVmcHand(poses, ovtr::SkeletalHandSide::Left);
    const ovtr::win32::ProfileSkeletonJoints rest = ovtr::win32::buildProfileSkeletonJoints(actor.profile);
    ovtr::win32::ProfileSkeletonJoints out = rest;
    ovtr::win32::MappingTransform wrist;
    wrist.position = {1.0f, 2.0f, 3.0f};
    require(
        ovtr::win32::applyVmcFingerHandFromPoses(out, rest, poses, false, {}, {},
            ovtr::win32::ProfileSkeletonHandSide::Left, wrist),
        "VMC finger solve applies valid hand"
    );
    require(
        out[ovtr::win32::kProfileJointLeftHandMiddle4].positionMeters.x > wrist.position.x + 0.05f,
        "VMC finger solve moves profile finger from wrist"
    );

    ovtr::win32::VmcLegRotationContinuity continuity;
    ovtr::win32::ProfileSkeletonJoints poseJoints = rest;
    poseJoints[ovtr::win32::kProfileJointLeftUpLeg].sideHint = {1.0f, 0.0f, 0.0f};
    ovtr::win32::SkeletonPose legPose = ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, poseJoints);
    const auto firstLegRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, legPose, continuity);
    poseJoints[ovtr::win32::kProfileJointLeftUpLeg].sideHint = {-1.0f, 0.0f, 0.0f};
    legPose = ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, poseJoints);
    const auto secondLegRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, legPose, continuity);
    require(
        quaternionAbsDot(
            firstLegRotations[ovtr::win32::kProfileJointLeftUpLeg],
            secondLegRotations[ovtr::win32::kProfileJointLeftUpLeg]
        ) > 0.99f,
        "VMC upper leg continuity ignores side hint roll flips"
    );

    poseJoints = rest;
    poseJoints[ovtr::win32::kProfileJointLeftUpLeg].sideHint = {-1.0f, 0.0f, 0.0f};
    legPose = ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, poseJoints);
    ovtr::win32::VmcLegRotationContinuity firstFrameFlippedContinuity;
    const auto firstFrameFlippedRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, legPose, firstFrameFlippedContinuity);
    require(
        quaternionAbsDot(
            firstFrameFlippedRotations[ovtr::win32::kProfileJointLeftUpLeg],
            firstLegRotations[ovtr::win32::kProfileJointLeftUpLeg]
        ) > 0.99f,
        "VMC upper leg first frame uses rest roll sign instead of raw flipped side hint"
    );

    ovtr::win32::VmcLegRotationContinuity twistedContinuity;
    twistedContinuity.valid[ovtr::win32::kProfileJointLeftUpLeg] = true;
    twistedContinuity.previousWorld[ovtr::win32::kProfileJointLeftUpLeg] = {0.0f, 1.0f, 0.0f, 0.0f};
    poseJoints = rest;
    poseJoints[ovtr::win32::kProfileJointLeftUpLeg].sideHint = {-1.0f, 0.0f, 0.0f};
    legPose = ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, poseJoints);
    const auto recoveredLegRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, legPose, twistedContinuity);
    ovtr::win32::VmcLegRotationContinuity freshContinuity;
    const auto freshLegRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, legPose, freshContinuity);
    require(
        quaternionAbsDot(
            recoveredLegRotations[ovtr::win32::kProfileJointLeftUpLeg],
            freshLegRotations[ovtr::win32::kProfileJointLeftUpLeg]
        ) > 0.99f,
        "VMC upper leg parent anchor recovers from a twisted previous roll"
    );

    poseJoints = rest;
    legPose = ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, poseJoints);
    ovtr::win32::VmcLegRotationContinuity rightRestContinuity;
    const auto rightRestRotations =
        ovtr::win32::makeVmcLocalRotationsWithContinuity(rest, poseJoints, legPose, rightRestContinuity);
    require(
        quaternionAbsDot(
            rightRestRotations[ovtr::win32::kProfileJointRightUpLeg],
            {0.0f, 0.0f, 0.0f, 1.0f}
        ) > 0.99f && quaternionAbsDot(rightRestRotations[ovtr::win32::kProfileJointLeftLeg], {0.0f, 0.0f, 0.0f, 1.0f}) > 0.99f &&
            quaternionAbsDot(rightRestRotations[ovtr::win32::kProfileJointLeftFoot], {0.0f, 0.0f, 0.0f, 1.0f}) > 0.99f,
        "VMC rest legs start without static 180 roll"
    );
    std::array<std::array<float, 4>, ovtr::win32::kProfileSkeletonJointCount> spacingRotations{};
    for (auto& rotation : spacingRotations) {
        rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    }
    ovtr::win32::applyVmcLimbSpacing(spacingRotations, 10.0f, 5.0f);
    require(
        quaternionAbsDot(
            spacingRotations[ovtr::win32::kProfileJointLeftArm],
            ovtr::quaternionFromEulerDegrees({0.0f, 0.0f, 10.0f})
        ) > 0.99f,
        "VMC arm spacing rotates left upper arm outward"
    );
    require(
        quaternionAbsDot(
            spacingRotations[ovtr::win32::kProfileJointRightUpLeg],
            ovtr::quaternionFromEulerDegrees({0.0f, 0.0f, -5.0f})
        ) > 0.99f,
        "VMC leg spacing mirrors right upper leg outward"
    );
}

} // namespace ovtr::test
#endif
