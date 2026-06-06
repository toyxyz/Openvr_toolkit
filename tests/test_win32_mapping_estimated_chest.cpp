#include "TestCases.h"

#ifdef _WIN32
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingModel.h"

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

void testWin32MappingEstimatedChest()
{
    using namespace ovtr::win32;
    MappingActor actor;
    const auto fullMapping = sequentialMapping();
    auto headPelvisMapping = defaultMappingDeviceRuntimeIndices();
    const int head = mappingSlotForRole(MappingTrackerRole::Head);
    const int pelvis = mappingSlotForRole(MappingTrackerRole::Pelvis);
    const int chest = mappingSlotForRole(MappingTrackerRole::Chest);
    headPelvisMapping[static_cast<std::size_t>(head)] = fullMapping[static_cast<std::size_t>(head)];
    headPelvisMapping[static_cast<std::size_t>(pelvis)] = fullMapping[static_cast<std::size_t>(pelvis)];

    const auto restTargets = mappingCalibrationRestTargets(actor.profile);
    const MappingCalibrationStatus status = captureMappingActorCalibration(
        actor,
        headPelvisMapping,
        posesForTargets(restTargets, fullMapping),
        false,
        {},
        {}
    );
    if (!status.success) {
        throw std::runtime_error("head/pelvis-only calibration should succeed");
    }
    if (actor.calibration.targetBindings[static_cast<std::size_t>(chest)].source !=
        MappingVirtualTargetSource::RestFallback) {
        throw std::runtime_error("unmapped chest should remain a rest fallback binding");
    }

    auto movedTargets = restTargets;
    movedTargets[static_cast<std::size_t>(head)].position.y += 0.2f;
    movedTargets[static_cast<std::size_t>(pelvis)].position.y += 0.2f;
    movedTargets[static_cast<std::size_t>(head)].rotation =
        ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    movedTargets[static_cast<std::size_t>(pelvis)].rotation =
        ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});

    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(movedTargets, fullMapping), false, {}, {})) {
        throw std::runtime_error("head/pelvis-only solve should estimate chest");
    }
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    requireNear(
        actor.liveJoints[kProfileJointSpine2].positionMeters.y,
        rest[kProfileJointSpine2].positionMeters.y + 0.2f,
        "estimated chest should follow head/pelvis vertical motion"
    );
    if (!actor.liveVirtualTargets[static_cast<std::size_t>(chest)].valid) {
        throw std::runtime_error("estimated chest virtual target should be valid");
    }
    if (std::fabs(actor.liveJoints[kProfileJointLeftShoulder].positionMeters.z) < 0.05f) {
        throw std::runtime_error("estimated chest yaw should rotate shoulder placement");
    }
}

} // namespace ovtr::test
#endif
