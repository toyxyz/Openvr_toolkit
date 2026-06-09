#ifdef _WIN32

#include "TestCases.h"
#include "TestSupport.h"

#include "math/QuaternionUtils.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonGltfHierarchy.h"
#include "platform/win32/SkeletonGltfPose.h"
#include "platform/win32/SkeletonGltfPoseBasis.h"
#include "platform/win32/SkeletonPose.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace ovtr::test {
namespace {

std::array<float, 4> axisAngle(const float x, const float y, const float z, const float degrees)
{
    constexpr float kPi = 3.14159265358979323846f;
    const float radians = degrees * kPi / 180.0f;
    const float s = std::sin(radians * 0.5f);
    return {x * s, y * s, z * s, std::cos(radians * 0.5f)};
}

void requireSameRotation(
    const std::array<float, 4>& a,
    const std::array<float, 4>& b,
    const char* message
) {
    const float dot = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
    require(dot > 0.999f, message);
}

void requireDifferentRotation(
    const std::array<float, 4>& a,
    const std::array<float, 4>& b,
    const char* message
) {
    const float dot = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
    require(dot < 0.99f, message);
}

void requireWorldAxisNear(
    const std::array<float, 4>& rotation,
    const ovtr::win32::Vec3 localAxis,
    const ovtr::win32::Vec3 expected,
    const char* message
) {
    const std::array<float, 3> actual = ovtr::rotatePositionByQuaternion(
        rotation,
        {localAxis.x, localAxis.y, localAxis.z}
    );
    require(std::abs(actual[0] - expected.x) < 0.001f &&
        std::abs(actual[1] - expected.y) < 0.001f &&
        std::abs(actual[2] - expected.z) < 0.001f, message);
}

ovtr::win32::Vec3 normalized(const ovtr::win32::Vec3 v)
{
    const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return length > 0.0001f
        ? ovtr::win32::Vec3{v.x / length, v.y / length, v.z / length}
        : ovtr::win32::Vec3{};
}

ovtr::win32::Vec3 direction(
    const ovtr::win32::ProfileSkeletonJoints& joints,
    const int parent,
    const int child
) {
    const ovtr::win32::Vec3 a = joints[static_cast<std::size_t>(parent)].positionMeters;
    const ovtr::win32::Vec3 b = joints[static_cast<std::size_t>(child)].positionMeters;
    return normalized({b.x - a.x, b.y - a.y, b.z - a.z});
}

float dotVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ovtr::win32::Vec3 projectOff(const ovtr::win32::Vec3 value, const ovtr::win32::Vec3 axis)
{
    const float dot = dotVec(value, axis);
    return {value.x - axis.x * dot, value.y - axis.y * dot, value.z - axis.z * dot};
}

float rollDeltaDegrees(const std::array<float, 4>& rest, const std::array<float, 4>& posed)
{
    constexpr float kPi = 3.14159265358979323846f;
    const std::array<float, 3> rotated = ovtr::rotatePositionByQuaternion(rest, {0.0f, 1.0f, 0.0f});
    const std::array<float, 4> delta = ovtr::multiplyQuaternion(posed, ovtr::conjugateQuaternion(rest));
    const float component = delta[0] * rotated[0] + delta[1] * rotated[1] + delta[2] * rotated[2];
    float degrees = 2.0f * std::atan2(component, delta[3]) * 180.0f / kPi;
    while (degrees > 180.0f) {
        degrees -= 360.0f;
    }
    while (degrees < -180.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

void requireSmallRollDelta(
    const std::array<float, 4>& rest,
    const std::array<float, 4>& posed,
    const char* message
) {
    require(std::abs(rollDeltaDegrees(rest, posed)) < 1.0f, message);
}

std::array<std::array<float, 4>, ovtr::win32::kProfileSkeletonJointCount> gltfWorldRotations(
    const ovtr::win32::ProfileSkeletonJoints& rest,
    const ovtr::win32::SkeletonPose& pose
) {
    using namespace ovtr::win32;
    std::array<std::array<float, 4>, kProfileSkeletonJointCount> world{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        const auto& local = pose.bones[static_cast<std::size_t>(joint)].localRotation;
        world[static_cast<std::size_t>(joint)] = parent >= 0
            ? ovtr::multiplyQuaternion(world[static_cast<std::size_t>(parent)], local)
            : local;
    }
    return world;
}

} // namespace

void testWin32SkeletonGltfFootFollowsToeDirection()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    const SkeletonPose restPose = makeRestSkeletonPose(rest);
    SkeletonPose footPose = restPose;
    footPose.bones[kProfileJointLeftFoot].localRotation = ovtr::normalizeQuaternion(
        ovtr::multiplyQuaternion(axisAngle(0.0f, 1.0f, 0.0f, 55.0f), axisAngle(1.0f, 0.0f, 0.0f, -30.0f))
    );
    footPose.bones[kProfileJointRightFoot].localRotation = ovtr::normalizeQuaternion(
        ovtr::multiplyQuaternion(axisAngle(0.0f, 1.0f, 0.0f, -45.0f), axisAngle(1.0f, 0.0f, 0.0f, 35.0f))
    );

    const ProfileSkeletonJoints sourceJoints =
        exportSkeletonGltfJoints(computeSkeletonPoseWorldJoints(rest, footPose));
    const std::vector<SkeletonPose> exported = makeSkeletonGltfExportPoses(rest, {restPose, footPose});
    const auto exportedWorld = gltfWorldRotations(rest, exported.back());
    requireWorldAxisNear(
        exportedWorld[kProfileJointLeftFoot],
        Vec3{0.0f, 1.0f, 0.0f},
        direction(sourceJoints, kProfileJointLeftFoot, kProfileJointLeftToeBase),
        "left foot should aim at toe endpoint"
    );
    requireWorldAxisNear(
        exportedWorld[kProfileJointRightFoot],
        Vec3{0.0f, 1.0f, 0.0f},
        direction(sourceJoints, kProfileJointRightFoot, kProfileJointRightToeBase),
        "right foot should aim at toe endpoint"
    );
}

void testWin32SkeletonGltfLegChainKeepsRestAndAnimates()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    const SkeletonPose restPose = makeRestSkeletonPose(rest);
    const SkeletonPose restExport = makeSkeletonGltfExportPose(rest, restPose, true);
    const auto restWorld = gltfWorldRotations(rest, restExport);
    const auto restSideAxes = computeSkeletonPoseWorldSideAxes(rest, restPose);
    requireWorldAxisNear(restWorld[kProfileJointLeftUpLeg], {1.0f, 0.0f, 0.0f}, restSideAxes[kProfileJointLeftLeg], "left upleg x rest");
    requireWorldAxisNear(restWorld[kProfileJointRightUpLeg], {1.0f, 0.0f, 0.0f}, restSideAxes[kProfileJointRightLeg], "right upleg x rest");
    requireWorldAxisNear(restWorld[kProfileJointLeftLeg], {1.0f, 0.0f, 0.0f}, restSideAxes[kProfileJointLeftFoot], "left leg x rest");
    requireWorldAxisNear(restWorld[kProfileJointRightLeg], {1.0f, 0.0f, 0.0f}, restSideAxes[kProfileJointRightFoot], "right leg x rest");

    SkeletonPose source = restPose;
    SkeletonPose lifted = restPose;
    lifted.bones[kProfileJointLeftUpLeg].localRotation = axisAngle(0.0f, 0.0f, 1.0f, 55.0f);
    lifted.bones[kProfileJointLeftLeg].localRotation = axisAngle(1.0f, 0.0f, 0.0f, 92.0f);
    lifted.bones[kProfileJointRightUpLeg].localRotation = axisAngle(0.0f, 0.0f, 1.0f, -48.0f);
    lifted.bones[kProfileJointRightLeg].localRotation = axisAngle(1.0f, 0.0f, 0.0f, 88.0f);

    const std::vector<SkeletonPose> exported = makeSkeletonGltfExportPoses(rest, {source, lifted});
    const auto firstWorld = gltfWorldRotations(rest, exported.front());
    const auto liftedWorld = gltfWorldRotations(rest, exported.back());
    const auto liftedSideAxes = computeSkeletonPoseWorldSideAxes(rest, lifted);

    requireDifferentRotation(firstWorld[kProfileJointLeftUpLeg], liftedWorld[kProfileJointLeftUpLeg], "left upleg should animate");
    requireDifferentRotation(firstWorld[kProfileJointLeftLeg], liftedWorld[kProfileJointLeftLeg], "left leg should animate");
    requireDifferentRotation(firstWorld[kProfileJointLeftFoot], liftedWorld[kProfileJointLeftFoot], "left foot should follow leg motion");
    requireDifferentRotation(firstWorld[kProfileJointRightUpLeg], liftedWorld[kProfileJointRightUpLeg], "right upleg should animate");
    requireDifferentRotation(firstWorld[kProfileJointRightLeg], liftedWorld[kProfileJointRightLeg], "right leg should animate");
    requireDifferentRotation(firstWorld[kProfileJointRightFoot], liftedWorld[kProfileJointRightFoot], "right foot should follow leg motion");
    requireWorldAxisNear(liftedWorld[kProfileJointLeftUpLeg], {1.0f, 0.0f, 0.0f}, liftedSideAxes[kProfileJointLeftLeg], "left upleg gltf source x");
    requireWorldAxisNear(liftedWorld[kProfileJointLeftLeg], {1.0f, 0.0f, 0.0f}, liftedSideAxes[kProfileJointLeftFoot], "left leg gltf source x");
    requireWorldAxisNear(liftedWorld[kProfileJointLeftFoot], {1.0f, 0.0f, 0.0f}, liftedSideAxes[kProfileJointLeftToeBase], "left foot gltf source x");
    requireWorldAxisNear(liftedWorld[kProfileJointRightUpLeg], {1.0f, 0.0f, 0.0f}, liftedSideAxes[kProfileJointRightLeg], "right upleg gltf source x");
    requireWorldAxisNear(liftedWorld[kProfileJointRightLeg], {1.0f, 0.0f, 0.0f}, liftedSideAxes[kProfileJointRightFoot], "right leg gltf source x");
    requireWorldAxisNear(liftedWorld[kProfileJointRightFoot], {1.0f, 0.0f, 0.0f}, liftedSideAxes[kProfileJointRightToeBase], "right foot gltf source x");

    SkeletonPose yawed = restPose;
    yawed.bones[kProfileJointHips].localRotation = axisAngle(0.0f, 1.0f, 0.0f, 80.0f);
    const std::vector<SkeletonPose> yawExported = makeSkeletonGltfExportPoses(rest, {restPose, yawed});
    requireSameRotation(
        yawExported.front().bones[kProfileJointLeftUpLeg].localRotation,
        yawExported.back().bones[kProfileJointLeftUpLeg].localRotation,
        "left upleg should inherit whole-body yaw from spine"
    );
    requireSameRotation(
        yawExported.front().bones[kProfileJointLeftLeg].localRotation,
        yawExported.back().bones[kProfileJointLeftLeg].localRotation,
        "left leg should inherit whole-body yaw from parent"
    );
    requireSmallRollDelta(
        yawExported.front().bones[kProfileJointLeftFoot].localRotation,
        yawExported.back().bones[kProfileJointLeftFoot].localRotation,
        "left foot should not add local roll during whole-body yaw"
    );
    requireSameRotation(
        yawExported.front().bones[kProfileJointRightUpLeg].localRotation,
        yawExported.back().bones[kProfileJointRightUpLeg].localRotation,
        "right upleg should inherit whole-body yaw from spine"
    );
    requireSameRotation(
        yawExported.front().bones[kProfileJointRightLeg].localRotation,
        yawExported.back().bones[kProfileJointRightLeg].localRotation,
        "right leg should inherit whole-body yaw from parent"
    );
    requireSmallRollDelta(
        yawExported.front().bones[kProfileJointRightFoot].localRotation,
        yawExported.back().bones[kProfileJointRightFoot].localRotation,
        "right foot should not add local roll during whole-body yaw"
    );

    SkeletonPose footRolled = restPose;
    footRolled.bones[kProfileJointLeftFoot].localRotation = axisAngle(0.0f, 1.0f, 0.0f, 70.0f);
    footRolled.bones[kProfileJointRightFoot].localRotation = axisAngle(0.0f, 1.0f, 0.0f, -70.0f);
    const std::vector<SkeletonPose> footRollExported = makeSkeletonGltfExportPoses(rest, {restPose, footRolled});
    requireSameRotation(
        footRollExported.front().bones[kProfileJointLeftLeg].localRotation,
        footRollExported.back().bones[kProfileJointLeftLeg].localRotation,
        "left lower leg should ignore foot roll"
    );
    requireSameRotation(
        footRollExported.front().bones[kProfileJointRightLeg].localRotation,
        footRollExported.back().bones[kProfileJointRightLeg].localRotation,
        "right lower leg should ignore foot roll"
    );
}

} // namespace ovtr::test

#endif
