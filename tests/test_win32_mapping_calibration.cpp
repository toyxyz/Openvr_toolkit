#include "TestCases.h"

#ifdef _WIN32
#include "data/SessionTypes.h"
#include "math/PoseTransform.h"
#include "platform/win32/AppProfileState.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/MappingCalibrationCapture.h"
#include "platform/win32/MappingCalibrationIk.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationTargets.h"
#include "platform/win32/MappingPoleSolve.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/MappingVirtualTargets.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

void requireNear(const float actual, const float expected, const char* message)
{
    if (std::fabs(actual - expected) > 0.001f) {
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

} // namespace

namespace ovtr::test {

void testWin32MappingCalibration()
{
    using namespace ovtr::win32;

    const MappingTransform transform{Vec3{1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f, 1.0f}};
    requireVecNear(
        composeMappingTransforms(inverseMappingTransform(transform), transform).position,
        Vec3{0.0f, 0.0f, 0.0f},
        "transform inverse should restore identity"
    );

    const TwoBoneIkResult ik = solveTwoBoneIk(
        Vec3{0.0f, 0.0f, 0.0f},
        Vec3{1.0f, 0.0f, 0.0f},
        Vec3{0.0f, 1.0f, 0.0f},
        1.0f,
        1.0f,
        Vec3{0.0f, 1.0f, 0.0f}
    );
    requireVecNear(ik.mid, Vec3{0.5f, 0.866025f, 0.0f}, "two-bone IK should place midpoint from pole");
    requireVecNear(ik.end, Vec3{1.0f, 0.0f, 0.0f}, "two-bone IK should preserve reachable target");

    const TwoBoneIkResult softIk = solveTwoBoneIk(
        Vec3{0.0f, 0.0f, 0.0f},
        Vec3{2.0f, 0.0f, 0.0f},
        Vec3{0.0f, 1.0f, 0.0f},
        1.0f,
        1.0f,
        Vec3{0.0f, 1.0f, 0.0f}
    );
    if (softIk.end.x >= 1.99f || softIk.mid.y <= 0.1f) {
        throw std::runtime_error("soft IK should avoid snapping to a fully straight limb at max reach");
    }
    const TwoBoneIkResult hardIk = solveTwoBoneIk(
        Vec3{0.0f, 0.0f, 0.0f},
        Vec3{2.0f, 0.0f, 0.0f},
        Vec3{0.0f, 1.0f, 0.0f},
        1.0f,
        1.0f,
        Vec3{0.0f, 1.0f, 0.0f},
        0.0f
    );
    requireVecNear(hardIk.end, Vec3{2.0f, 0.0f, 0.0f}, "zero soft IK should preserve hard max reach");

    MappingActor actor;
    if (mappingSlotForRole(MappingTrackerRole::LeftHand) != 5 ||
        mappingRoleForSlot(10) != MappingTrackerRole::RightFoot) {
        throw std::runtime_error("mapping role/slot order should stay stable");
    }
    const auto mapping = sequentialMapping();
    auto targets = mappingCalibrationRestTargets(actor.profile);
    requireNear(targets[0].position.y, 1.60f, "head target should anchor at Head joint");
    requireNear(targets[5].position.x, 0.775f, "left hand target should be on character-left side");
    requireNear(targets[6].position.x, -0.775f, "right hand target should be on character-right side");
    const ovtr::PosePollResult validPoses = posesForTargets(targets, mapping);

    MappingCalibrationStatus status = captureMappingActorCalibration(
        actor,
        defaultMappingDeviceRuntimeIndices(),
        validPoses,
        false,
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    );
    if (status.success || status.message.find(L"Head") == std::wstring::npos) {
        throw std::runtime_error("calibration should fail on None mapping with slot label");
    }

    auto duplicateMapping = mapping;
    duplicateMapping[1] = duplicateMapping[0];
    status = captureMappingActorCalibration(actor, duplicateMapping, validPoses, false, {}, {});
    if (status.success) {
        throw std::runtime_error("calibration should fail on duplicated runtime index");
    }

    ovtr::PosePollResult invalidPoses = validPoses;
    invalidPoses.poses[2].flags = ovtr::PoseFlagDeviceConnected;
    status = captureMappingActorCalibration(actor, mapping, invalidPoses, false, {}, {});
    if (status.success) {
        throw std::runtime_error("calibration should fail on invalid pose");
    }

    status = captureMappingActorCalibration(actor, mapping, validPoses, false, {}, {});
    if (!status.success || !actor.calibrated || actor.calibration.runtimeIndices[0] != 1u) {
        throw std::runtime_error("calibration should store runtime-index snapshot");
    }
    requireNear(actor.calibration.armSoftIkStrength, kDefaultMappingSoftIkStrength, "default arm soft IK snapshot");
    requireNear(actor.calibration.legSoftIkStrength, kDefaultMappingSoftIkStrength, "default leg soft IK snapshot");
    requireVecNear(actor.calibration.trackerToTarget[0].position, Vec3{}, "rest calibration offset should be identity");

    MappingActor customSoftActor;
    status = captureMappingActorCalibration(customSoftActor, mapping, validPoses, false, {}, {}, 0.0f, 0.15f);
    if (!status.success) {
        throw std::runtime_error("custom soft IK calibration should succeed");
    }
    requireNear(customSoftActor.calibration.armSoftIkStrength, 0.0f, "custom arm soft IK snapshot");
    requireNear(customSoftActor.calibration.legSoftIkStrength, 0.15f, "custom leg soft IK snapshot");

    std::array<MappingVirtualTarget, kMappingSlotCount> virtualTargets{};
    MappingVirtualTargetBuildResult targetResult = buildMappingVirtualTargets(
        actor.calibration,
        validPoses,
        false,
        {},
        {},
        virtualTargets
    );
    if (!targetResult.success || virtualTargets[0].role != MappingTrackerRole::Head) {
        throw std::runtime_error("virtual targets should restore role-tagged rest targets");
    }
    requireNear(virtualTargets[0].transform.position.y, 1.60f, "head virtual target y");
    targetResult = buildMappingVirtualTargets(actor.calibration, invalidPoses, false, {}, {}, virtualTargets);
    if (targetResult.success || targetResult.failedSlot != 2) {
        throw std::runtime_error("virtual target build should report invalid pose slot");
    }

    targets[2].position.y += 0.10f;
    targets[1].position.y += 0.10f;
    targets[0].position.y += 0.10f;
    ovtr::PosePollResult movedPoses = posesForTargets(targets, mapping);
    if (!updateCalibratedMappingActorJoints(actor, movedPoses, false, {}, {})) {
        throw std::runtime_error("live solve should update from valid poses");
    }
    requireNear(actor.liveJoints[kProfileJointHips].positionMeters.y, 1.0f, "pelvis tracker should move hips");
    requireNear(actor.liveJoints[kProfileJointSpine2].positionMeters.y, 1.53f, "chest tracker should move Spine2");
    requireNear(actor.liveJoints[kProfileJointNeck].positionMeters.y, 1.60f, "neck should keep head-local offset");
    requireNear(actor.liveJoints[kProfileJointHead].positionMeters.y, 1.70f, "head tracker should move head");
    const Vec3 lastLeftWrist = actor.liveJoints[kProfileJointLeftForeArm].positionMeters;

    auto partialLostTargets = targets;
    partialLostTargets[2].position.y += 0.05f;
    partialLostTargets[5].position.x += 0.20f;
    ovtr::PosePollResult partialLostPoses = posesForTargets(partialLostTargets, mapping);
    partialLostPoses.poses[5].flags = ovtr::PoseFlagDeviceConnected;
    if (!updateCalibratedMappingActorJoints(actor, partialLostPoses, false, {}, {})) {
        throw std::runtime_error("live solve should continue with one lost tracker");
    }
    if (!actor.liveTrackingLost) {
        throw std::runtime_error("partial lost tracker should set tracking-lost status");
    }
    requireNear(actor.liveJoints[kProfileJointHips].positionMeters.y, 1.05f, "valid pelvis should keep updating");
    requireVecNear(
        actor.liveJoints[kProfileJointLeftForeArm].positionMeters,
        lastLeftWrist,
        "lost hand tracker should hold last valid wrist target"
    );

    auto rotatedChestTargets = mappingCalibrationRestTargets(actor.profile);
    rotatedChestTargets[1].rotation = ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    rotatedChestTargets[2].rotation = ovtr::quaternionFromEulerDegrees({0.0f, 90.0f, 0.0f});
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(rotatedChestTargets, mapping), false, {}, {})) {
        throw std::runtime_error("torso rotation live solve should update");
    }
    requireNear(actor.liveJoints[kProfileJointLeftShoulder].positionMeters.z, -0.205f, "chest yaw should rotate left shoulder");
    requireNear(actor.liveJoints[kProfileJointLeftUpLeg].positionMeters.z, -0.14f, "pelvis yaw should rotate left hip");

    auto pitchedChestTargets = mappingCalibrationRestTargets(actor.profile);
    pitchedChestTargets[1].rotation = ovtr::quaternionFromEulerDegrees({30.0f, 0.0f, 0.0f});
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(pitchedChestTargets, mapping), false, {}, {})) {
        throw std::runtime_error("chest pitch live solve should update");
    }
    requireNear(actor.liveJoints[kProfileJointSpine2].positionMeters.z, 0.0f, "Spine2 should stay on chest target");
    requireNear(actor.liveJoints[kProfileJointSpine].positionMeters.z, -0.0202f, "Spine should partially follow chest pitch");
    requireNear(actor.liveJoints[kProfileJointSpine1].positionMeters.z, -0.0393f, "Spine1 should bend more toward chest pitch");

    auto rotatedHeadTargets = mappingCalibrationRestTargets(actor.profile);
    rotatedHeadTargets[0].position.x += 0.20f;
    rotatedHeadTargets[0].rotation = {0.0f, 0.0f, 0.70710677f, 0.70710677f};
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(rotatedHeadTargets, mapping), false, {}, {})) {
        throw std::runtime_error("head rotation live solve should update");
    }
    requireNear(actor.liveJoints[kProfileJointNeck].positionMeters.x, 0.30f, "head rotation should move neck opposite head end");
    requireNear(actor.liveJoints[kProfileJointHead].positionMeters.x, 0.20f, "head tracker should move Head joint");
    requireNear(actor.liveJoints[kProfileJointHeadTopEnd].positionMeters.x, 0.10f, "head rotation should move head end around Head");

    auto fallbackTargets = mappingCalibrationRestTargets(actor.profile);
    fallbackTargets[3].position = fallbackTargets[5].position;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(fallbackTargets, mapping), false, {}, {})) {
        throw std::runtime_error("pole fallback live solve should update");
    }
    const int leftArmPole = mappingPoleIndex(MappingPoleKind::LeftArm);
    if (!actor.liveDebugPoles[static_cast<std::size_t>(leftArmPole)].fallback || !actor.liveLimited) {
        throw std::runtime_error("collinear pole should use fallback and set limited status");
    }

    auto limitedTargets = mappingCalibrationRestTargets(actor.profile);
    limitedTargets[5].position.x += 10.0f;
    if (!updateCalibratedMappingActorJoints(actor, posesForTargets(limitedTargets, mapping), false, {}, {})) {
        throw std::runtime_error("reach-limited live solve should update");
    }
    if (!actor.liveDebugPoles[static_cast<std::size_t>(leftArmPole)].limited || !actor.liveLimited) {
        throw std::runtime_error("unreachable hand target should set limited status");
    }
    requireNear(actor.liveJoints[kProfileJointLeftForeArm].positionMeters.x, 0.775f, "wrist should clamp to max reach");

    MappingActor offsetActor;
    auto offsetTargets = mappingCalibrationRestTargets(offsetActor.profile);
    offsetTargets[5].position.z += 0.10f;
    status = captureMappingActorCalibration(
        offsetActor,
        mapping,
        posesForTargets(offsetTargets, mapping),
        false,
        {},
        {}
    );
    if (!status.success) {
        throw std::runtime_error("offset calibration should succeed");
    }
    offsetTargets[5].position.x -= 0.04f;
    if (!updateCalibratedMappingActorJoints(offsetActor, posesForTargets(offsetTargets, mapping), false, {}, {})) {
        throw std::runtime_error("offset live solve should update");
    }
    requireNear(offsetActor.liveJoints[kProfileJointLeftForeArm].positionMeters.z, 0.0f, "wrist should use tracker offset");
    requireNear(offsetActor.liveJoints[kProfileJointLeftForeArm].positionMeters.x, 0.735f, "wrist target should move after offset");

    resetMappingActorCalibration(offsetActor);
    if (offsetActor.calibrated ||
        offsetActor.liveJointsValid ||
        offsetActor.liveTrackingLost ||
        offsetActor.liveLimited) {
        throw std::runtime_error("reset should clear calibrated actor runtime status");
    }
    if (offsetActor.calibration.runtimeIndices[0] != kNoSelectedRuntimeIndex ||
        offsetActor.liveVirtualTargets[0].valid) {
        throw std::runtime_error("reset should clear calibration mapping and live targets");
    }
    if (updateCalibratedMappingActorJoints(offsetActor, posesForTargets(offsetTargets, mapping), false, {}, {})) {
        throw std::runtime_error("reset actor should not track until recalibrated");
    }
}

} // namespace ovtr::test
#endif
