#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingCalibrationIk.h"
#include "platform/win32/MappingArmRollHints.h"
#include "platform/win32/MappingCoreSolve.h"
#include "platform/win32/MappingEstimatedArmPole.h"
#include "platform/win32/MappingEstimatedChest.h"
#include "platform/win32/MappingFingerSolve.h"
#include "platform/win32/MappingLegRollHints.h"
#include "platform/win32/MappingPinnedTargets.h"
#include "platform/win32/MappingPoleSolve.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/MappingVirtualTargets.h"
#include "platform/win32/SkeletonFingerRoll.h"
#include "platform/win32/SkeletonPose.h"

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

MappingTransform targetFor(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    return targets[static_cast<std::size_t>(mappingSlotForRole(role))].transform;
}

Vec3 restDelta(const ProfileSkeletonJoints& joints, const int child, const int parent) noexcept
{
    return subMappingVec3(joints[child].positionMeters, joints[parent].positionMeters);
}

Vec3 add(const Vec3 a, const Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

float restDistance(const ProfileSkeletonJoints& joints, const int child, const int parent) noexcept
{
    return distanceMappingVec3(joints[child].positionMeters, joints[parent].positionMeters);
}

void resetFrameState(MappingActor& actor) noexcept
{
    actor.liveTrackingLost = false;
    actor.liveLimited = false;
    actor.liveDebugPoles = {};
}

MappingPoleSolveResult solvePoleForLimb(
    MappingActor& actor,
    const MappingPoleKind poleKind,
    const Vec3 root,
    const Vec3 target,
    const Vec3 hint,
    const Vec3 restMid,
    const float upperLength,
    const float lowerLength
) noexcept {
    const int index = mappingPoleIndex(poleKind);
    MappingPoleSolveResult pole = solveMappingPole(MappingPoleSolveInput{
        root,
        target,
        hint,
        restMid,
        actor.livePoleDirections[static_cast<std::size_t>(index)],
        actor.livePoleDirectionValid[static_cast<std::size_t>(index)],
        upperLength,
        lowerLength
    });
    actor.livePoleDirections[static_cast<std::size_t>(index)] = pole.direction;
    actor.livePoleDirectionValid[static_cast<std::size_t>(index)] = true;
    actor.liveDebugPoles[static_cast<std::size_t>(index)] = pole.debug;
    actor.liveLimited = actor.liveLimited || pole.fallback || pole.limited;
    return pole;
}

void solveArm(
    MappingActor& actor,
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const bool left
) {
    const int shoulder = left ? kProfileJointLeftShoulder : kProfileJointRightShoulder;
    const int elbow = left ? kProfileJointLeftArm : kProfileJointRightArm;
    const int wrist = left ? kProfileJointLeftForeArm : kProfileJointRightForeArm;
    const MappingTransform poleTarget = targetFor(targets, left ? MappingTrackerRole::LeftArm : MappingTrackerRole::RightArm);
    const MappingTransform wristTarget = targetFor(targets, left ? MappingTrackerRole::LeftHand : MappingTrackerRole::RightHand);
    const float upperLength = restDistance(rest, elbow, shoulder);
    const float lowerLength = restDistance(rest, wrist, elbow);
    const MappingPoleSolveResult pole = solvePoleForLimb(
        actor,
        left ? MappingPoleKind::LeftArm : MappingPoleKind::RightArm,
        out[shoulder].positionMeters,
        wristTarget.position,
        poleTarget.position,
        rest[elbow].positionMeters,
        upperLength,
        lowerLength
    );
    const TwoBoneIkResult ik = solveTwoBoneIk(
        out[shoulder].positionMeters,
        wristTarget.position,
        pole.polePoint,
        upperLength,
        lowerLength,
        rest[elbow].positionMeters,
        actor.calibration.armSoftIkStrength
    );
    out[elbow].positionMeters = ik.mid;
    out[wrist].positionMeters = actor.calibration.pinHandTargets ? wristTarget.position : ik.end;
}

void solveLeg(
    MappingActor& actor,
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const bool left
) {
    const int hip = left ? kProfileJointLeftUpLeg : kProfileJointRightUpLeg;
    const int knee = left ? kProfileJointLeftLeg : kProfileJointRightLeg;
    const int ankle = left ? kProfileJointLeftFoot : kProfileJointRightFoot;
    const int toe = left ? kProfileJointLeftToeBase : kProfileJointRightToeBase;
    const MappingTransform poleTarget = targetFor(targets, left ? MappingTrackerRole::LeftLeg : MappingTrackerRole::RightLeg);
    const MappingTransform ankleTarget = targetFor(targets, left ? MappingTrackerRole::LeftFoot : MappingTrackerRole::RightFoot);
    const float upperLength = restDistance(rest, knee, hip);
    const float lowerLength = restDistance(rest, ankle, knee);
    const MappingPoleSolveResult pole = solvePoleForLimb(
        actor,
        left ? MappingPoleKind::LeftLeg : MappingPoleKind::RightLeg,
        out[hip].positionMeters,
        ankleTarget.position,
        poleTarget.position,
        rest[knee].positionMeters,
        upperLength,
        lowerLength
    );
    const TwoBoneIkResult ik = solveTwoBoneIk(
        out[hip].positionMeters,
        ankleTarget.position,
        pole.polePoint,
        upperLength,
        lowerLength,
        rest[knee].positionMeters,
        actor.calibration.legSoftIkStrength
    );
    out[knee].positionMeters = ik.mid;
    out[ankle].positionMeters = actor.calibration.pinFootTargets ? ankleTarget.position : ik.end;
    out[toe].positionMeters = transformMappingPoint(ankleTarget, restDelta(rest, toe, ankle));
}

Vec3 targetSide(const std::array<MappingVirtualTarget, kMappingSlotCount>& targets, const MappingTrackerRole role) noexcept { return rotateMappingVector(targetFor(targets, role), Vec3{1.0f, 0.0f, 0.0f}); }
Vec3 targetUp(const std::array<MappingVirtualTarget, kMappingSlotCount>& targets, const MappingTrackerRole role) noexcept { return rotateMappingVector(targetFor(targets, role), Vec3{0.0f, 1.0f, 0.0f}); }
Vec3 targetForward(const std::array<MappingVirtualTarget, kMappingSlotCount>& targets, const MappingTrackerRole role) noexcept { return rotateMappingVector(targetFor(targets, role), Vec3{0.0f, 0.0f, 1.0f}); }

void applyStableRollHints(
    ProfileSkeletonJoints& out,
    const ProfileSkeletonJoints& rest,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const std::array<Vec3, kProfileSkeletonJointCount>* previousSideAxes
) noexcept
{
    out[kProfileJointHips].sideHint = targetSide(targets, MappingTrackerRole::Pelvis);
    out[kProfileJointHips].upHint = targetUp(targets, MappingTrackerRole::Pelvis);
    out[kProfileJointSpine].sideHint = targetSide(targets, MappingTrackerRole::Pelvis);
    out[kProfileJointSpine1].sideHint =
        add(targetSide(targets, MappingTrackerRole::Pelvis), targetSide(targets, MappingTrackerRole::Chest));
    out[kProfileJointSpine2].sideHint = targetSide(targets, MappingTrackerRole::Chest);
    out[kProfileJointLeftShoulder].sideHint = targetForward(targets, MappingTrackerRole::Chest);
    out[kProfileJointRightShoulder].sideHint = scaleMappingVec3(targetForward(targets, MappingTrackerRole::Chest), -1.0f);
    out[kProfileJointNeck].sideHint = targetSide(targets, MappingTrackerRole::Head);
    out[kProfileJointHead].sideHint = targetSide(targets, MappingTrackerRole::Head);
    out[kProfileJointHeadTopEnd].sideHint = targetSide(targets, MappingTrackerRole::Head);

    applyMappingLegRollHints(out, rest, targets, previousSideAxes);
    applyMappingArmRollHints(out, rest, targets, previousSideAxes);
    out[kProfileJointLeftHand].sideHint = targetForward(targets, MappingTrackerRole::LeftHand);
    out[kProfileJointRightHand].sideHint = targetForward(targets, MappingTrackerRole::RightHand);
}

} // namespace

bool updateCalibratedMappingActorJoints(
    MappingActor& actor,
    const ovtr::PosePollResult& poses,
    const bool originEnabled,
    const std::array<float, 3>& originOffset,
    const std::array<float, 3>& originRotationDegrees
) {
    if (!actor.calibrated) {
        return false;
    }
    std::array<MappingVirtualTarget, kMappingSlotCount> targets{};
    const MappingVirtualTargetBuildResult targetResult = buildMappingVirtualTargetsWithFallback(
        actor.calibration,
        poses,
        originEnabled,
        originOffset,
        originRotationDegrees,
        actor.liveVirtualTargets,
        targets
    );
    if (!targetResult.success) {
        actor.liveTrackingLost = true;
        actor.livePoleTargetValid = {};
        actor.liveEstimatedArmPoleLocalDirectionValid = {};
        actor.liveEstimatedArmHandLocalPositionValid = {};
        return false;
    }

    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    applyEstimatedChestTarget(actor.calibration, rest, targets);

    resetFrameState(actor);
    actor.liveTrackingLost = targetResult.trackingLost;
    ProfileSkeletonJoints out = rest;
    const auto previousSideAxes = actor.liveJointsValid
        ? computeSkeletonPoseWorldSideAxes(rest, actor.liveSkeletonPose)
        : std::array<Vec3, kProfileSkeletonJointCount>{};
    const auto* previousSideAxesPtr = actor.liveJointsValid ? &previousSideAxes : nullptr;
    const MappingCoreSolveResult core = solveMappingCoreJoints(rest, targets, out);
    actor.liveLimited = core.limited;
    applyEstimatedArmPoleTargets(
        actor.calibration,
        rest,
        out,
        actor.liveEstimatedArmPoleLocalDirections,
        actor.liveEstimatedArmPoleLocalDirectionValid,
        actor.liveEstimatedArmHandLocalPositions,
        actor.liveEstimatedArmHandLocalPositionValid,
        targets
    );
    actor.liveVirtualTargets = targets;
    solveArm(actor, out, rest, targets, true);
    solveArm(actor, out, rest, targets, false);
    solveLeg(actor, out, rest, targets, true);
    solveLeg(actor, out, rest, targets, false);
    updateMappingActorFingerJoints(
        actor,
        out,
        rest,
        poses,
        originEnabled,
        originOffset,
        originRotationDegrees,
        targets
    );
    applyStableRollHints(out, rest, targets, previousSideAxesPtr);
    actor.liveSkeletonPose = makeSkeletonPoseFromWorldJoints(rest, out);
    stabilizeSkeletonConnectorRolls(rest, actor.liveSkeletonPose);
    stabilizeSkeletonFingerRolls(rest, actor.liveSkeletonPose);
    applyPinnedMappingTargets(actor.calibration, rest, targets, actor.liveSkeletonPose);
    actor.liveJoints = computeSkeletonPoseWorldJoints(rest, actor.liveSkeletonPose);
    actor.liveJointsValid = true;
    // Temporary CSV diagnostics remain in MappingCalibrationPoseDebugLog.* for future reuse.
    return true;
}

} // namespace ovtr::win32
