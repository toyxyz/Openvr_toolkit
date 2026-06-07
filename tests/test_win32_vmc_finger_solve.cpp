#include "TestCases.h"
#include "TestSupport.h"

#ifdef _WIN32
#include "data/VmcSyntheticPose.h"
#include "math/PoseTransform.h"
#include "platform/win32/MappingVmcFingerSolve.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonFingerRoll.h"
#include "platform/win32/SkeletonPose.h"
#include "platform/win32/VmcFingerDisplayRotation.h"
#include "platform/win32/VmcFingerOutputRotation.h"
#include "platform/win32/VmcFingerRotationFilter.h"
#include "platform/win32/SkeletonGltfPoseBasis.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <string>

namespace {

float dotVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ovtr::win32::Vec3 subVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b) noexcept
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

ovtr::win32::Vec3 normalizedVec(const ovtr::win32::Vec3 value) noexcept
{
    const float length = std::sqrt(dotVec(value, value));
    return length > 0.00001f
        ? ovtr::win32::Vec3{value.x / length, value.y / length, value.z / length}
        : ovtr::win32::Vec3{};
}

float dotArray3(const std::array<float, 3>& a, const std::array<float, 3>& b) noexcept
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

std::array<float, 3> normalizedArray3(const std::array<float, 3>& value) noexcept
{
    const float length = std::sqrt(dotArray3(value, value));
    return length > 0.00001f
        ? std::array<float, 3>{value[0] / length, value[1] / length, value[2] / length}
        : std::array<float, 3>{};
}

std::array<float, 3> projectedPalmArray3(
    const std::array<float, 3>& palm,
    const std::array<float, 3>& axis
) noexcept {
    const float amount = dotArray3(palm, axis);
    return normalizedArray3({
        palm[0] - axis[0] * amount,
        palm[1] - axis[1] * amount,
        palm[2] - axis[2] * amount
    });
}

std::array<float, 4> axisAngle(const std::array<float, 3>& axis, const float degrees) noexcept
{
    constexpr float kPi = 3.14159265358979323846f;
    const float half = degrees * kPi / 360.0f;
    return {axis[0] * std::sin(half), axis[1] * std::sin(half), axis[2] * std::sin(half), std::cos(half)};
}

ovtr::win32::Vec3 projectedPalmSide(
    const ovtr::win32::ProfileSkeletonJoints& joints,
    const int joint
) noexcept {
    const int parent = joints[static_cast<std::size_t>(joint)].parentIndex;
    const ovtr::win32::Vec3 axis = normalizedVec(subVec(
        joints[static_cast<std::size_t>(joint)].positionMeters,
        joints[static_cast<std::size_t>(parent)].positionMeters
    ));
    const ovtr::win32::Vec3 side = joints[static_cast<std::size_t>(joint)].sideHint;
    const float amount = dotVec(side, axis);
    return normalizedVec({side.x - axis.x * amount, side.y - axis.y * amount, side.z - axis.z * amount});
}

void appendVmcPose(
    ovtr::PosePollResult& result,
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
    result.poses.push_back(pose);
}

void appendFinger(
    ovtr::PosePollResult& result,
    const ovtr::SkeletalHandSide side,
    const std::uint32_t finger,
    const float spread,
    const bool curled
) {
    const std::uint32_t first = 1u + finger * 4u;
    if (!curled) {
        for (std::uint32_t segment = 0; segment < 4u; ++segment) {
            appendVmcPose(result, side, first + segment, {spread, 0.0f, 0.03f + 0.025f * segment});
        }
        return;
    }
    appendVmcPose(result, side, first + 0u, {spread, 0.0f, 0.030f});
    appendVmcPose(result, side, first + 1u, {spread, -0.025f, 0.050f});
    appendVmcPose(result, side, first + 2u, {spread, -0.055f, 0.055f});
    appendVmcPose(result, side, first + 3u, {spread, -0.080f, 0.045f});
}

ovtr::PosePollResult bentVmcHand()
{
    ovtr::PosePollResult result;
    appendVmcPose(result, ovtr::SkeletalHandSide::Left, 0, {});
    appendFinger(result, ovtr::SkeletalHandSide::Left, 0, 0.045f, false);
    appendFinger(result, ovtr::SkeletalHandSide::Left, 1, 0.025f, false);
    appendFinger(result, ovtr::SkeletalHandSide::Left, 2, 0.000f, true);
    appendFinger(result, ovtr::SkeletalHandSide::Left, 3, -0.020f, false);
    appendFinger(result, ovtr::SkeletalHandSide::Left, 4, -0.040f, false);
    return result;
}

ovtr::PosePollResult longBaseBentVmcHand()
{
    ovtr::PosePollResult result;
    appendVmcPose(result, ovtr::SkeletalHandSide::Left, 0, {});
    appendFinger(result, ovtr::SkeletalHandSide::Left, 0, 0.045f, false);
    appendFinger(result, ovtr::SkeletalHandSide::Left, 1, 0.025f, false);
    const std::uint32_t middleFirst = 1u + 2u * 4u;
    appendVmcPose(result, ovtr::SkeletalHandSide::Left, middleFirst + 0u, {0.000f, 0.000f, 0.120f});
    appendVmcPose(result, ovtr::SkeletalHandSide::Left, middleFirst + 1u, {0.000f, -0.030f, 0.145f});
    appendVmcPose(result, ovtr::SkeletalHandSide::Left, middleFirst + 2u, {0.000f, -0.060f, 0.150f});
    appendVmcPose(result, ovtr::SkeletalHandSide::Left, middleFirst + 3u, {0.000f, -0.085f, 0.140f});
    appendFinger(result, ovtr::SkeletalHandSide::Left, 3, -0.020f, false);
    appendFinger(result, ovtr::SkeletalHandSide::Left, 4, -0.040f, false);
    return result;
}

} // namespace

namespace ovtr::test {

void testWin32VmcFingerSolve()
{
    const std::array<float, 3> segmentParent{0.0f, 0.0f, 0.0f};
    const std::array<float, 3> segmentChild{0.0f, -0.03f, 0.04f};
    const std::array<float, 3> palmSide{0.0f, 1.0f, 0.0f};
    const std::array<float, 4> displayRotation =
        ovtr::win32::vmcFingerDisplayRotationForSegment(segmentParent, segmentChild, palmSide);
    const std::array<float, 3> displayY =
        ovtr::rotatePositionByQuaternion(displayRotation, {0.0f, 1.0f, 0.0f});
    const std::array<float, 3> displayX =
        ovtr::rotatePositionByQuaternion(displayRotation, {1.0f, 0.0f, 0.0f});
    const std::array<float, 3> segmentAxis = normalizedArray3(segmentChild);
    require(dotArray3(displayY, segmentAxis) > 0.99f, "VMC display finger Y follows bent segment");
    require(
        dotArray3(displayX, projectedPalmArray3(palmSide, segmentAxis)) > 0.99f,
        "VMC display finger roll stays palm-oriented while bent"
    );
    const std::array<float, 3> distalParent{0.0f, -0.055f, 0.055f};
    const std::array<float, 3> distalTip{0.0f, -0.080f, 0.045f};
    const std::array<float, 4> distalRotation =
        ovtr::win32::vmcFingerDisplayRotationForSegment(distalParent, distalTip, palmSide);
    const std::array<float, 3> distalY =
        ovtr::rotatePositionByQuaternion(distalRotation, {0.0f, 1.0f, 0.0f});
    require(
        dotArray3(distalY, normalizedArray3({0.0f, -0.025f, -0.010f})) > 0.99f,
        "VMC distal display finger Y follows distal-to-tip direction"
    );
    const std::array<float, 4> pureRoll =
        ovtr::win32::removeVmcFingerLocalRoll(axisAngle({0.0f, 0.0f, 1.0f}, 180.0f), {0.0f, 0.0f, 1.0f});
    require(
        dotArray3(ovtr::rotatePositionByQuaternion(pureRoll, {1.0f, 0.0f, 0.0f}), {1.0f, 0.0f, 0.0f}) > 0.99f,
        "VMC pure finger roll is removed"
    );
    const std::array<float, 4> curlThenRoll = ovtr::multiplyQuaternion(
        axisAngle({1.0f, 0.0f, 0.0f}, 45.0f),
        axisAngle({0.0f, 0.0f, 1.0f}, 90.0f)
    );
    const std::array<float, 4> curlOnly =
        ovtr::win32::removeVmcFingerLocalRoll(curlThenRoll, {0.0f, 0.0f, 1.0f});
    require(
        dotArray3(
            ovtr::rotatePositionByQuaternion(curlOnly, {0.0f, 0.0f, 1.0f}),
            ovtr::rotatePositionByQuaternion(curlThenRoll, {0.0f, 0.0f, 1.0f})
        ) > 0.99f,
        "VMC curl/spread direction survives roll removal"
    );

    ovtr::win32::BodyProfile profile;
    const ovtr::win32::ProfileSkeletonJoints rest =
        ovtr::win32::buildProfileSkeletonJoints(profile);
    ovtr::win32::ProfileSkeletonJoints out = rest;
    ovtr::win32::MappingTransform wrist;
    require(
        ovtr::win32::applyVmcFingerHandFromPoses(
            out,
            rest,
            bentVmcHand(),
            false,
            {},
            {},
            ovtr::win32::ProfileSkeletonHandSide::Left,
            wrist
        ),
        "VMC bent finger solve succeeds"
    );
    require(
        out[ovtr::win32::kProfileJointLeftHandMiddle1].positionMeters.y > 0.01f,
        "VMC proximal curl bends the first profile finger joint"
    );
    ovtr::win32::ProfileSkeletonJoints longBaseOut = rest;
    require(
        ovtr::win32::applyVmcFingerHandFromPoses(
            longBaseOut,
            rest,
            longBaseBentVmcHand(),
            false,
            {},
            {},
            ovtr::win32::ProfileSkeletonHandSide::Left,
            wrist
        ),
        "VMC long-base bent finger solve succeeds"
    );
    require(
        longBaseOut[ovtr::win32::kProfileJointLeftHandMiddle1].positionMeters.y > 0.01f,
        "VMC long proximal-origin offset still bends first profile finger joint"
    );
    ovtr::win32::SkeletonPose pose = ovtr::win32::makeSkeletonPoseFromWorldJoints(rest, out);
    std::array<std::array<float, 4>, ovtr::win32::kProfileSkeletonJointCount> vmcLocal{};
    for (int joint = 0; joint < ovtr::win32::kProfileSkeletonJointCount; ++joint) {
        vmcLocal[static_cast<std::size_t>(joint)] =
            pose.bones[static_cast<std::size_t>(joint)].localRotation;
    }
    auto vmcWorld = ovtr::win32::computeSkeletonPoseWorldRotations(rest, pose);
    const int middle2 = ovtr::win32::kProfileJointLeftHandMiddle2;
    ovtr::win32::applyVmcFingerOutputRotations(rest, out, vmcLocal, vmcWorld);
    const ovtr::win32::SkeletonGltfBasis restBasis =
        ovtr::win32::skeletonGltfExportBasisFor(rest, middle2);
    const ovtr::win32::SkeletonGltfBasis poseBasis =
        ovtr::win32::vmcFingerOutputPoseBasisForJoint(
            rest, out, vmcWorld, ovtr::win32::ProfileSkeletonHandSide::Left, middle2);
    require(
        dotVec(ovtr::win32::skeletonWorldAxis(vmcWorld[middle2], restBasis.y), poseBasis.y) > 0.99f,
        "VMC output finger local FK follows solved finger direction"
    );
    ovtr::win32::stabilizeSkeletonFingerRolls(rest, pose);
    const auto sideAxes = ovtr::win32::computeSkeletonPoseWorldSideAxes(rest, pose);
    const ovtr::win32::Vec3 expectedSide =
        projectedPalmSide(out, ovtr::win32::kProfileJointLeftHandMiddle1);
    const float rollDot = dotVec(
        sideAxes[ovtr::win32::kProfileJointLeftHandMiddle1],
        expectedSide
    );
    require(
        rollDot > 0.95f,
        "VMC finger roll stays palm-oriented after proximal curl: " + std::to_string(rollDot)
    );
}

} // namespace ovtr::test
#endif
