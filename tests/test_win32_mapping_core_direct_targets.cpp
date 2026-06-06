#include "TestCases.h"

#ifdef _WIN32
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingTransformMath.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace {

void requireNear(const float actual, const float expected, const char* message)
{
    if (std::fabs(actual - expected) > 0.002f) {
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

ovtr::win32::Vec3 restDelta(
    const ovtr::win32::ProfileSkeletonJoints& rest,
    const int child,
    const int parent
) {
    return ovtr::win32::subMappingVec3(rest[child].positionMeters, rest[parent].positionMeters);
}

} // namespace

namespace ovtr::test {

void testWin32MappingCoreDirectTargets()
{
    using namespace ovtr::win32;
    MappingActor actor;
    const auto mapping = sequentialMapping();
    const auto restTargets = mappingCalibrationRestTargets(actor.profile);
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    const MappingCalibrationStatus status = captureMappingActorCalibration(
        actor,
        mapping,
        posesForTargets(restTargets, mapping),
        false,
        {},
        {}
    );
    if (!status.success) {
        throw std::runtime_error("direct-target test calibration should succeed");
    }

    auto chestFar = restTargets;
    const std::size_t chest = static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::Chest));
    chestFar[chest].position.y += 1.0f;
    chestFar[chest].position.z += 0.5f;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(chestFar, mapping), false, {}, {})) {
        throw std::runtime_error("far chest target solve should update");
    }
    requireVecNear(actor.liveJoints[kProfileJointSpine2].positionMeters, chestFar[chest].position,
        "Spine2 should attach to the chest target");

    auto headFar = restTargets;
    const std::size_t head = static_cast<std::size_t>(mappingSlotForRole(MappingTrackerRole::Head));
    headFar[head].position.x += 0.5f;
    headFar[head].position.y += 0.5f;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(headFar, mapping), false, {}, {})) {
        throw std::runtime_error("far head target solve should update");
    }
    requireVecNear(actor.liveJoints[kProfileJointHead].positionMeters, headFar[head].position,
        "Head should attach to the head target");

    auto rotated = restTargets;
    rotated[chest].rotation = ovtr::quaternionFromEulerDegrees({12.0f, 40.0f, 18.0f});
    rotated[head].rotation = ovtr::quaternionFromEulerDegrees({-15.0f, 25.0f, 60.0f});
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(rotated, mapping), false, {}, {})) {
        throw std::runtime_error("rotated chest/head solve should update");
    }
    requireVecNear(actor.liveJoints[kProfileJointLeftShoulder].positionMeters,
        transformMappingPoint(rotated[chest], restDelta(rest, kProfileJointLeftShoulder, kProfileJointSpine2)),
        "Shoulder should use the chest transform");
    requireVecNear(actor.liveJoints[kProfileJointNeck].positionMeters,
        transformMappingPoint(rotated[head], restDelta(rest, kProfileJointNeck, kProfileJointHead)),
        "Neck should use the head transform");
    requireVecNear(actor.liveJoints[kProfileJointHeadTopEnd].positionMeters,
        transformMappingPoint(rotated[head], restDelta(rest, kProfileJointHeadTopEnd, kProfileJointHead)),
        "Head end should use the head transform");
}

} // namespace ovtr::test
#endif
