#ifdef _WIN32

#include "TestCases.h"
#include "TestSupport.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/ProfileSkeleton.h"
#include "platform/win32/SkeletonFingerRoll.h"
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

ovtr::win32::Vec3 rotatedX(const std::array<float, 4>& rotation)
{
    const std::array<float, 3> axis = ovtr::rotatePositionByQuaternion(rotation, {1.0f, 0.0f, 0.0f});
    return ovtr::win32::Vec3{axis[0], axis[1], axis[2]};
}

ovtr::win32::Vec3 rotatedY(const std::array<float, 4>& rotation)
{
    const std::array<float, 3> axis = ovtr::rotatePositionByQuaternion(rotation, {0.0f, 1.0f, 0.0f});
    return ovtr::win32::Vec3{axis[0], axis[1], axis[2]};
}

ovtr::win32::Vec3 rotatedZ(const std::array<float, 4>& rotation)
{
    const std::array<float, 3> axis = ovtr::rotatePositionByQuaternion(rotation, {0.0f, 0.0f, 1.0f});
    return ovtr::win32::Vec3{axis[0], axis[1], axis[2]};
}

ovtr::win32::Vec3 rotateVec(const std::array<float, 4>& rotation, const ovtr::win32::Vec3 value)
{
    const std::array<float, 3> out = ovtr::rotatePositionByQuaternion(rotation, {value.x, value.y, value.z});
    return ovtr::win32::Vec3{out[0], out[1], out[2]};
}

ovtr::win32::Vec3 subVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

ovtr::win32::Vec3 addVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

float dotVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ovtr::win32::Vec3 scaleVec(const ovtr::win32::Vec3 value, const float scale)
{
    return {value.x * scale, value.y * scale, value.z * scale};
}

ovtr::win32::Vec3 crossVec(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

ovtr::win32::Vec3 normalizeVec(const ovtr::win32::Vec3 value)
{
    const float length = std::sqrt(dotVec(value, value));
    return length > 0.00001f ? scaleVec(value, 1.0f / length) : ovtr::win32::Vec3{0.0f, 1.0f, 0.0f};
}

ovtr::win32::Vec3 projectOff(const ovtr::win32::Vec3 value, const ovtr::win32::Vec3 axis)
{
    return subVec(value, scaleVec(axis, dotVec(value, axis)));
}

void requireVecNear(const ovtr::win32::Vec3 a, const ovtr::win32::Vec3 b, const char* message)
{
    require(std::abs(a.x - b.x) < 0.001f &&
        std::abs(a.y - b.y) < 0.001f &&
        std::abs(a.z - b.z) < 0.001f, message);
}

void requireSameRotation(
    const std::array<float, 4>& a,
    const std::array<float, 4>& b,
    const char* message
) {
    const float dot = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
    require(dot > 0.999f, message);
}

float rotationAngleDegrees(const std::array<float, 4>& a, const std::array<float, 4>& b)
{
    constexpr float kPi = 3.14159265358979323846f;
    float dot = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
    if (dot > 1.0f) {
        dot = 1.0f;
    }
    return 2.0f * std::acos(dot) * 180.0f / kPi;
}

std::array<ovtr::win32::Vec3, ovtr::win32::kProfileSkeletonJointCount> gltfWorldPositions(
    const ovtr::win32::ProfileSkeletonJoints& rest,
    const ovtr::win32::SkeletonPose& bindPose,
    const ovtr::win32::SkeletonPose& pose,
    const std::array<std::array<float, 4>, ovtr::win32::kProfileSkeletonJointCount>& rotations
) {
    using namespace ovtr::win32;
    std::array<Vec3, kProfileSkeletonJointCount> positions{};
    for (int joint = 0; joint < kProfileSkeletonJointCount; ++joint) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        const Vec3 local = joint == kProfileJointSpine
            ? pose.bones[static_cast<std::size_t>(joint)].localTranslationMeters
            : bindPose.bones[static_cast<std::size_t>(joint)].localTranslationMeters;
        positions[static_cast<std::size_t>(joint)] = parent >= 0
            ? addVec(positions[static_cast<std::size_t>(parent)], rotateVec(rotations[parent], local))
            : local;
    }
    return positions;
}

} // namespace

void testWin32SkeletonGltfHandRollFollowsForearm()
{
    using namespace ovtr::win32;

    const BodyProfile profile;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(profile);
    const SkeletonPose restPose = makeRestSkeletonPose(rest);
    const SkeletonPose restExport = makeSkeletonGltfExportPose(rest, restPose, true);
    const auto restWorld = gltfWorldRotations(rest, restExport);
    const auto restSide = computeSkeletonPoseWorldSideAxes(rest, restPose);

    requireVecNear(rotatedZ(restWorld[kProfileJointLeftShoulder]), Vec3{0.0f, 1.0f, 0.0f}, "left shoulder roll axis");
    requireVecNear(rotatedZ(restWorld[kProfileJointLeftArm]), Vec3{0.0f, 1.0f, 0.0f}, "left upper arm roll axis");
    requireVecNear(rotatedZ(restWorld[kProfileJointLeftForeArm]), Vec3{0.0f, 1.0f, 0.0f}, "left forearm roll axis");
    requireVecNear(rotatedZ(restWorld[kProfileJointRightShoulder]), Vec3{0.0f, 1.0f, 0.0f}, "right shoulder roll axis");
    requireVecNear(rotatedZ(restWorld[kProfileJointRightArm]), Vec3{0.0f, 1.0f, 0.0f}, "right upper arm roll axis");
    requireVecNear(rotatedZ(restWorld[kProfileJointRightForeArm]), Vec3{0.0f, 1.0f, 0.0f}, "right forearm roll axis");
    require(dotVec(rotatedX(restWorld[kProfileJointLeftForeArm]), restSide[kProfileJointLeftForeArm]) > 0.999f, "left forearm bind x follows source side");
    require(dotVec(rotatedX(restWorld[kProfileJointRightForeArm]), restSide[kProfileJointRightForeArm]) > 0.999f, "right forearm bind x follows source side");

    const Vec3 leftHandX = normalizeVec(projectOff(rotatedX(restWorld[kProfileJointLeftForeArm]), rotatedY(restWorld[kProfileJointLeftHand])));
    const Vec3 rightHandX = normalizeVec(projectOff(rotatedX(restWorld[kProfileJointRightForeArm]), rotatedY(restWorld[kProfileJointRightHand])));
    requireVecNear(rotatedX(restWorld[kProfileJointLeftHand]), leftHandX, "left hand x follows projected forearm roll");
    requireVecNear(rotatedX(restWorld[kProfileJointRightHand]), rightHandX, "right hand x follows projected forearm roll");

    SkeletonPose yawed = restPose;
    yawed.bones[kProfileJointHips].localRotation = axisAngle(0.0f, 1.0f, 0.0f, 85.0f);
    const std::vector<SkeletonPose> yawExported = makeSkeletonGltfExportPoses(rest, {restPose, yawed});
    requireSameRotation(yawExported.front().bones[kProfileJointLeftHand].localRotation, yawExported.back().bones[kProfileJointLeftHand].localRotation, "left hand inherits yaw");
    requireSameRotation(yawExported.front().bones[kProfileJointRightHand].localRotation, yawExported.back().bones[kProfileJointRightHand].localRotation, "right hand inherits yaw");

    SkeletonPose armed = restPose;
    armed.bones[kProfileJointLeftArm].localRotation = ovtr::normalizeQuaternion({0.7f, 0.051f, -0.1f, 0.705f});
    armed.bones[kProfileJointRightArm].localRotation = ovtr::normalizeQuaternion({0.7f, -0.051f, 0.1f, 0.705f});
    const std::vector<SkeletonPose> armExported = makeSkeletonGltfExportPoses(rest, {armed});
    const auto sourceSide = computeSkeletonPoseWorldSideAxes(rest, armed);
    const auto armWorld = gltfWorldRotations(rest, armExported.front());
    require(dotVec(rotatedX(armWorld[kProfileJointLeftArm]), sourceSide[kProfileJointLeftArm]) > 0.999f, "left upper arm GLB x follows source side axis");
    require(dotVec(rotatedX(armWorld[kProfileJointRightArm]), sourceSide[kProfileJointRightArm]) > 0.999f, "right upper arm GLB x follows source side axis");

    SkeletonPose firstBent = restPose;
    firstBent.bones[kProfileJointLeftShoulder].localRotation =
        ovtr::normalizeQuaternion({0.089847f, 0.001019f, 0.011292f, 0.995891f});
    firstBent.bones[kProfileJointLeftArm].localRotation =
        ovtr::normalizeQuaternion({-0.001442f, -0.385576f, -0.223463f, 0.895206f});
    firstBent.bones[kProfileJointLeftForeArm].localRotation =
        ovtr::normalizeQuaternion({0.574849f, 0.0f, 0.818260f, 0.0f});
    firstBent.bones[kProfileJointLeftHand].localRotation =
        ovtr::normalizeQuaternion({0.938280f, -0.331343f, 0.058729f, 0.079958f});
    const std::vector<SkeletonPose> firstBentExported = makeSkeletonGltfExportPoses(rest, {firstBent});
    const SkeletonPose firstBentBind = makeSkeletonGltfExportPose(rest, restPose, true);
    require(
        rotationAngleDegrees(
            firstBentBind.bones[kProfileJointLeftForeArm].localRotation,
            firstBentExported.front().bones[kProfileJointLeftForeArm].localRotation
        ) < 130.0f,
        "first bent left forearm export should keep the rest-pre-roll roll branch"
    );
    require(
        rotationAngleDegrees(
            firstBentBind.bones[kProfileJointLeftHand].localRotation,
            firstBentExported.front().bones[kProfileJointLeftHand].localRotation
        ) < 130.0f,
        "first bent left hand export should keep the rest-pre-roll roll branch"
    );

    ProfileSkeletonJoints curledJoints = rest;
    curledJoints[kProfileJointLeftHandMiddle2].positionMeters.y -= 0.04f;
    curledJoints[kProfileJointLeftHandMiddle3].positionMeters.y -= 0.08f;
    curledJoints[kProfileJointLeftHandMiddle4].positionMeters.y -= 0.12f;
    SkeletonPose curled = makeSkeletonPoseFromWorldJoints(rest, curledJoints);
    stabilizeSkeletonFingerRolls(rest, curled);
    const ProfileSkeletonJoints viewportJoints = computeSkeletonPoseWorldJoints(rest, curled);
    const ProfileSkeletonJoints gltfJoints = exportSkeletonGltfJoints(viewportJoints);
    const std::vector<SkeletonPose> fingerExported = makeSkeletonGltfExportPoses(rest, {curled});
    const SkeletonPose bindPose = makeSkeletonGltfExportPose(rest, makeRestSkeletonPose(rest), true);
    const auto fingerWorld = gltfWorldRotations(rest, fingerExported.front());
    const auto fingerPositions = gltfWorldPositions(rest, bindPose, fingerExported.front(), fingerWorld);
    const Vec3 forward = normalizeVec(subVec(
        gltfJoints[kProfileJointLeftHandMiddle1].positionMeters,
        gltfJoints[kProfileJointLeftHand].positionMeters
    ));
    const Vec3 spread = normalizeVec(projectOff(subVec(
        gltfJoints[kProfileJointLeftHandIndex1].positionMeters,
        gltfJoints[kProfileJointLeftHandPinky1].positionMeters
    ), forward));
    const Vec3 palm = normalizeVec(crossVec(spread, forward));
    require(dotVec(rotatedZ(fingerWorld[kProfileJointLeftHand]), palm) > 0.95f, "hand GLB parent axis should stay on hand palm axis");
    for (const int joint : {kProfileJointLeftHandMiddle2, kProfileJointLeftHandMiddle3, kProfileJointLeftHandMiddle4}) {
        const int parent = skeletonGltfParentIndex(rest, joint);
        const Vec3 expectedY = normalizeVec(subVec(gltfJoints[joint].positionMeters, gltfJoints[parent].positionMeters));
        const Vec3 exportedY = normalizeVec(subVec(fingerPositions[joint], fingerPositions[parent]));
        require(dotVec(exportedY, expectedY) > 0.999f, "finger GLB connected bone direction should match viewport pose");
    }
}

} // namespace ovtr::test

#endif
