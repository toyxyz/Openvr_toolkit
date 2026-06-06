#include "TestCases.h"

#ifdef _WIN32
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/MappingVirtualTargets.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace {

void requireNear(const float actual, const float expected, const char* message)
{
    if (std::fabs(actual - expected) > 0.001f) {
        throw std::runtime_error(message);
    }
}

void requireTransformNear(
    const ovtr::win32::MappingTransform& actual,
    const ovtr::win32::MappingTransform& expected,
    const char* message
) {
    requireNear(actual.position.x, expected.position.x, message);
    requireNear(actual.position.y, expected.position.y, message);
    requireNear(actual.position.z, expected.position.z, message);
    for (std::size_t index = 0; index < actual.rotation.size(); ++index) {
        requireNear(actual.rotation[index], expected.rotation[index], message);
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
        if (mapping[index] == ovtr::win32::kNoSelectedRuntimeIndex) {
            continue;
        }
        ovtr::PoseSample pose;
        pose.runtimeIndex = mapping[index];
        pose.position = {targets[index].position.x, targets[index].position.y, targets[index].position.z};
        pose.rotation = targets[index].rotation;
        pose.flags = ovtr::PoseFlagDeviceConnected | ovtr::PoseFlagPoseValid;
        result.poses.push_back(pose);
    }
    return result;
}

} // namespace

namespace ovtr::test {

void testWin32MappingLegParentFallback()
{
    using namespace ovtr::win32;
    MappingActor actor;
    const int leftLeg = mappingSlotForRole(MappingTrackerRole::LeftLeg);
    const int leftFoot = mappingSlotForRole(MappingTrackerRole::LeftFoot);
    const auto fullMapping = sequentialMapping();
    auto footOnlyMapping = defaultMappingDeviceRuntimeIndices();
    footOnlyMapping[static_cast<std::size_t>(leftFoot)] =
        fullMapping[static_cast<std::size_t>(leftFoot)];

    const auto restTargets = mappingCalibrationRestTargets(actor.profile);
    const MappingCalibrationStatus status = captureMappingActorCalibration(
        actor,
        footOnlyMapping,
        posesForTargets(restTargets, fullMapping),
        false,
        {},
        {}
    );
    if (!status.success) {
        throw std::runtime_error("foot-only leg fallback calibration should succeed");
    }
    const auto& binding = actor.calibration.targetBindings[static_cast<std::size_t>(leftLeg)];
    if (binding.source != MappingVirtualTargetSource::ParentedTracker || binding.parentSlot != leftFoot) {
        throw std::runtime_error("missing leg pole should parent to the foot slot");
    }

    auto movedTargets = restTargets;
    movedTargets[static_cast<std::size_t>(leftFoot)].position.x += 0.25f;
    movedTargets[static_cast<std::size_t>(leftFoot)].rotation =
        ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    std::array<MappingVirtualTarget, kMappingSlotCount> targets{};
    const MappingVirtualTargetBuildResult result = buildMappingVirtualTargetsWithFallback(
        actor.calibration,
        posesForTargets(movedTargets, fullMapping),
        false,
        {},
        {},
        actor.liveVirtualTargets,
        targets
    );
    if (!result.success || result.trackingLost) {
        throw std::runtime_error("parented leg pole should build from valid foot pose");
    }
    const MappingTransform expected = composeMappingTransforms(
        movedTargets[static_cast<std::size_t>(leftFoot)],
        actor.calibration.trackerToTarget[static_cast<std::size_t>(leftLeg)]
    );
    requireTransformNear(targets[static_cast<std::size_t>(leftLeg)].transform, expected, "leg pole should follow foot transform");

    ovtr::PosePollResult invalidFootPoses = posesForTargets(movedTargets, fullMapping);
    for (ovtr::PoseSample& pose : invalidFootPoses.poses) {
        if (pose.runtimeIndex == footOnlyMapping[static_cast<std::size_t>(leftFoot)]) {
            pose.flags = ovtr::PoseFlagDeviceConnected;
        }
    }
    const MappingVirtualTargetBuildResult lostResult = buildMappingVirtualTargetsWithFallback(
        actor.calibration,
        invalidFootPoses,
        false,
        {},
        {},
        targets,
        targets
    );
    if (!lostResult.success || !lostResult.trackingLost) {
        throw std::runtime_error("invalid parent foot should keep previous leg pole fallback");
    }
    requireTransformNear(targets[static_cast<std::size_t>(leftLeg)].transform, expected, "lost foot should hold previous leg pole");
}

} // namespace ovtr::test
#endif
