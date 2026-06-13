#include "TestCases.h"

#ifdef _WIN32
#include "data/SessionTypes.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/SkeletonPose.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

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

void requireNear(const float actual, const float expected, const char* message)
{
    if (std::fabs(actual - expected) > 0.001f) {
        throw std::runtime_error(message);
    }
}

void requireVecNear(const ovtr::win32::Vec3 actual, const ovtr::win32::Vec3 expected, const char* message)
{
    if (std::fabs(actual.x - expected.x) > 0.001f) {
        throw std::runtime_error(std::string(message) + " x");
    }
    if (std::fabs(actual.y - expected.y) > 0.001f) {
        throw std::runtime_error(std::string(message) + " y");
    }
    if (std::fabs(actual.z - expected.z) > 0.001f) {
        throw std::runtime_error(std::string(message) + " z");
    }
}

void requireAxisNear(ovtr::win32::Vec3 actual, ovtr::win32::Vec3 expected, const char* message)
{
    using namespace ovtr::win32;
    actual = normalizeMappingVec3Or(actual, {1.0f, 0.0f, 0.0f});
    expected = normalizeMappingVec3Or(expected, {1.0f, 0.0f, 0.0f});
    const float dot = dotMappingVec3(actual, expected);
    if (dot < 0.98f) {
        throw std::runtime_error(std::string(message) + " dot=" + std::to_string(dot));
    }
}

} // namespace

namespace ovtr::test {

void testWin32MappingPelvisRotation()
{
    using namespace ovtr::win32;

    MappingActor actor;
    const auto mapping = sequentialMapping();
    auto targets = mappingCalibrationRestTargets(actor.profile);
    const PosePollResult calibrationPoses = posesForTargets(targets, mapping);
    const MappingCalibrationStatus status =
        captureMappingActorCalibration(actor, mapping, calibrationPoses, false, {}, {});
    if (!status.success) {
        throw std::runtime_error("pelvis rotation test calibration failed");
    }

    const std::size_t pelvisSlot =
        static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::Pelvis));
    targets[pelvisSlot].rotation = ovtr::quaternionFromEulerDegrees({25.0f, 35.0f, -20.0f});
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(targets, mapping), false, {}, {})) {
        throw std::runtime_error("pelvis rotation live solve failed");
    }

    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    const auto rotations = computeSkeletonPoseWorldRotations(rest, actor.liveSkeletonPose);
    const Vec3 hipsUp = skeletonWorldAxis(rotations[kProfileJointHips], {0.0f, 1.0f, 0.0f});
    const Vec3 hipsSide = skeletonWorldAxis(rotations[kProfileJointHips], {1.0f, 0.0f, 0.0f});
    requireAxisNear(hipsUp, rotateMappingVector(targets[pelvisSlot], {0.0f, 1.0f, 0.0f}), "Hips up should follow pelvis pitch/roll");
    requireAxisNear(hipsSide, rotateMappingVector(targets[pelvisSlot], {1.0f, 0.0f, 0.0f}), "Hips side should follow pelvis rotation");

    const auto sideAxes = computeSkeletonPoseWorldSideAxes(rest, actor.liveSkeletonPose);
    requireAxisNear(
        sideAxes[kProfileJointSpine],
        rotateMappingVector(targets[pelvisSlot], {1.0f, 0.0f, 0.0f}),
        "Spine side should follow pelvis rotation"
    );

    const Vec3 expectedLeftHip =
        transformMappingPoint(targets[pelvisSlot], subMappingVec3(rest[kProfileJointLeftUpLeg].positionMeters, rest[kProfileJointHips].positionMeters));
    const Vec3 expectedRightHip =
        transformMappingPoint(targets[pelvisSlot], subMappingVec3(rest[kProfileJointRightUpLeg].positionMeters, rest[kProfileJointHips].positionMeters));
    const Vec3 expectedSpine =
        transformMappingPoint(targets[pelvisSlot], subMappingVec3(rest[kProfileJointSpine].positionMeters, rest[kProfileJointHips].positionMeters));
    requireVecNear(actor.liveJoints[kProfileJointLeftUpLeg].positionMeters, expectedLeftHip, "left hip should use full pelvis rotation");
    requireVecNear(actor.liveJoints[kProfileJointRightUpLeg].positionMeters, expectedRightHip, "right hip should use full pelvis rotation");
    requireVecNear(actor.liveJoints[kProfileJointSpine].positionMeters, expectedSpine, "Spine should use full pelvis rotation");
}

} // namespace ovtr::test
#endif
