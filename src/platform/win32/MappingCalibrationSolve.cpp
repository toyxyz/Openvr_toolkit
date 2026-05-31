#include "platform/win32/MappingCalibrationSolve.h"

#include "platform/win32/MappingCalibrationIk.h"
#include "platform/win32/MappingCoreSolve.h"
#include "platform/win32/MappingFingerSolve.h"
#include "platform/win32/MappingPoleSolve.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/MappingVirtualTargets.h"

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
    out[wrist].positionMeters = ik.end;
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
    out[ankle].positionMeters = ik.end;
    out[toe].positionMeters = transformMappingPoint(ankleTarget, restDelta(rest, toe, ankle));
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
        return false;
    }

    resetFrameState(actor);
    actor.liveTrackingLost = targetResult.trackingLost;
    actor.liveVirtualTargets = targets;
    const ProfileSkeletonJoints rest = buildProfileSkeletonJoints(actor.profile);
    ProfileSkeletonJoints out = rest;
    const MappingCoreSolveResult core = solveMappingCoreJoints(rest, targets, out);
    actor.liveLimited = core.limited;
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
    actor.liveJoints = out;
    actor.liveJointsValid = true;
    return true;
}

} // namespace ovtr::win32
