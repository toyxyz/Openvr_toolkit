#include "TestCases.h"

#ifdef _WIN32
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingElbowPosePrior.h"
#include "platform/win32/MappingPoleSolve.h"
#include "platform/win32/MappingTransformMath.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

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

void testWin32MappingEstimatedArmPole()
{
    using namespace ovtr::win32;
    const MappingTransform chestIdentity;
    const Vec3 leftClosePrior = sampleElbowPosePreferredDirection(
        chestIdentity,
        Vec3{0.04f, -0.04f, 0.10f},
        true
    );
    const Vec3 rightClosePrior = sampleElbowPosePreferredDirection(
        chestIdentity,
        Vec3{-0.04f, -0.04f, 0.10f},
        false
    );
    if (leftClosePrior.x < 0.50f || rightClosePrior.x > -0.50f) {
        throw std::runtime_error("close-chest elbow pose prior should point outside for each arm");
    }
    if (leftClosePrior.y > -0.10f || rightClosePrior.y > -0.10f ||
        leftClosePrior.z > -0.20f || rightClosePrior.z > -0.20f) {
        throw std::runtime_error("close-chest elbow pose prior should bias down and back");
    }
    Vec3 previousSlowPrior = sampleElbowPosePreferredDirection(
        chestIdentity,
        Vec3{0.12f, -0.22f, -0.28f},
        true
    );
    for (int step = 1; step <= 40; ++step) {
        const float y = -0.22f + 0.44f * static_cast<float>(step) / 40.0f;
        const Vec3 slowPrior = sampleElbowPosePreferredDirection(
            chestIdentity,
            Vec3{0.12f, y, -0.28f},
            true
        );
        if (dotMappingVec3(previousSlowPrior, slowPrior) < 0.998f) {
            throw std::runtime_error("behind-body elbow pose prior should stay continuous during slow vertical hand motion");
        }
        previousSlowPrior = slowPrior;
    }

    MappingActor actor;
    actor.livePoleTargetValid[static_cast<std::size_t>(mappingPoleIndex(MappingPoleKind::LeftArm))] = true;
    const auto fullMapping = sequentialMapping();
    auto mapping = defaultMappingDeviceRuntimeIndices();
    const int head = mappingSlotForRole(MappingTrackerRole::Head);
    const int chest = mappingSlotForRole(MappingTrackerRole::Chest);
    const int pelvis = mappingSlotForRole(MappingTrackerRole::Pelvis);
    const int leftArm = mappingSlotForRole(MappingTrackerRole::LeftArm);
    const int leftHand = mappingSlotForRole(MappingTrackerRole::LeftHand);
    mapping[static_cast<std::size_t>(head)] = fullMapping[static_cast<std::size_t>(head)];
    mapping[static_cast<std::size_t>(pelvis)] = fullMapping[static_cast<std::size_t>(pelvis)];
    mapping[static_cast<std::size_t>(leftHand)] = fullMapping[static_cast<std::size_t>(leftHand)];

    const auto restTargets = mappingCalibrationRestTargets(actor.profile);
    const MappingCalibrationStatus status = captureMappingActorCalibration(
        actor,
        mapping,
        posesForTargets(restTargets, fullMapping),
        false,
        {},
        {}
    );
    if (!status.success) {
        throw std::runtime_error("head/pelvis/hand calibration should succeed without arm tracker");
    }
    if (actor.calibration.targetBindings[static_cast<std::size_t>(leftArm)].source !=
        MappingVirtualTargetSource::RestFallback) {
        throw std::runtime_error("unmapped arm should remain a rest fallback binding");
    }
    const std::size_t leftPole = static_cast<std::size_t>(mappingPoleIndex(MappingPoleKind::LeftArm));
    if (actor.livePoleTargetValid[leftPole]) {
        throw std::runtime_error("calibration should clear stale arm pole target history");
    }

    auto movedTargets = restTargets;
    movedTargets[static_cast<std::size_t>(leftHand)].position.z -= 0.25f;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(movedTargets, fullMapping), false, {}, {})) {
        throw std::runtime_error("arm solve should work with estimated elbow pole");
    }
    if (!actor.livePoleTargetValid[leftPole] ||
        distanceMappingVec3(actor.livePoleTargets[leftPole], movedTargets[static_cast<std::size_t>(leftHand)].position) > 0.0001f) {
        throw std::runtime_error("estimated arm pole should remember the previous hand target");
    }
    const Vec3 shoulder = actor.liveJoints[kProfileJointLeftShoulder].positionMeters;
    const Vec3 wrist = actor.liveVirtualTargets[static_cast<std::size_t>(leftHand)].transform.position;
    const Vec3 pole = actor.liveVirtualTargets[static_cast<std::size_t>(leftArm)].transform.position;
    const Vec3 armAxis = normalizeMappingVec3Or(subMappingVec3(wrist, shoulder), Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 poleDir = normalizeMappingVec3Or(subMappingVec3(pole, shoulder), Vec3{0.0f, 0.0f, -1.0f});
    if (std::fabs(dotMappingVec3(armAxis, poleDir)) > 0.01f) {
        throw std::runtime_error("estimated arm pole should be perpendicular to shoulder-wrist axis");
    }
    if (distanceMappingVec3(pole, restTargets[static_cast<std::size_t>(leftArm)].position) < 0.05f) {
        throw std::runtime_error("estimated arm pole should replace the rest fallback target");
    }

    movedTargets[static_cast<std::size_t>(leftHand)].position =
        addMappingVec3(restTargets[static_cast<std::size_t>(chest)].position, Vec3{0.08f, -0.10f, -0.05f});
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(movedTargets, fullMapping), false, {}, {})) {
        throw std::runtime_error("near-chest arm solve should work with estimated elbow pole");
    }
    const Vec3 nearPoleA = normalizeMappingVec3Or(
        subMappingVec3(actor.liveVirtualTargets[static_cast<std::size_t>(leftArm)].transform.position, shoulder),
        poleDir
    );
    movedTargets[static_cast<std::size_t>(leftHand)].position.x += 0.015f;
    movedTargets[static_cast<std::size_t>(leftHand)].position.z += 0.010f;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(movedTargets, fullMapping), false, {}, {})) {
        throw std::runtime_error("near-chest arm solve should stay valid after small hand motion");
    }
    const Vec3 nearPoleB = normalizeMappingVec3Or(
        subMappingVec3(actor.liveVirtualTargets[static_cast<std::size_t>(leftArm)].transform.position, shoulder),
        nearPoleA
    );
    if (dotMappingVec3(nearPoleA, nearPoleB) < 0.75f) {
        throw std::runtime_error("near-chest estimated arm pole should not spin on small hand motion");
    }

    const Vec3 halfFoldBase = addMappingVec3(
        actor.liveJoints[kProfileJointLeftShoulder].positionMeters,
        Vec3{0.16f, -0.12f, 0.14f}
    );
    movedTargets[static_cast<std::size_t>(leftHand)].position = halfFoldBase;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(movedTargets, fullMapping), false, {}, {})) {
        throw std::runtime_error("half-fold arm solve should work with estimated elbow pole");
    }
    const Vec3 halfFoldPoleA = normalizeMappingVec3Or(
        subMappingVec3(
            actor.liveVirtualTargets[static_cast<std::size_t>(leftArm)].transform.position,
            actor.liveJoints[kProfileJointLeftShoulder].positionMeters
        ),
        nearPoleB
    );
    movedTargets[static_cast<std::size_t>(leftHand)].position =
        addMappingVec3(halfFoldBase, Vec3{0.08f, 0.05f, -0.11f});
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(movedTargets, fullMapping), false, {}, {})) {
        throw std::runtime_error("half-fold fast arm solve should stay valid");
    }
    const Vec3 halfFoldPoleB = normalizeMappingVec3Or(
        subMappingVec3(
            actor.liveVirtualTargets[static_cast<std::size_t>(leftArm)].transform.position,
            actor.liveJoints[kProfileJointLeftShoulder].positionMeters
        ),
        halfFoldPoleA
    );
    if (dotMappingVec3(halfFoldPoleA, halfFoldPoleB) < 0.60f) {
        throw std::runtime_error("half-fold estimated arm pole should not clatter on fast hand motion");
    }
}

} // namespace ovtr::test
#endif
