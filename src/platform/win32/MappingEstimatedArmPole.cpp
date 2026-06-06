#include "platform/win32/MappingEstimatedArmPole.h"

#include "platform/win32/MappingElbowPosePrior.h"
#include "platform/win32/MappingPoleSolve.h"
#include "platform/win32/MappingTransformMath.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace ovtr::win32 {
namespace {

constexpr float kAntiTorsoDistance = 0.45f;
constexpr float kAntiTorsoBlend = 0.70f;
constexpr float kPi = 3.14159265358979323846f;

int slotIndex(const MappingTrackerRole role) noexcept
{
    return static_cast<int>(mappingSlotForRole(role));
}

Vec3 projectOffAxis(const Vec3 value, const Vec3 axis) noexcept
{
    return subMappingVec3(value, scaleMappingVec3(axis, dotMappingVec3(value, axis)));
}

float saturate(const float value) noexcept
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

float degreesToRadians(const float degrees) noexcept
{
    return degrees * kPi / 180.0f;
}

Vec3 blendDirection(const Vec3 base, const Vec3 target, const float amount) noexcept
{
    return normalizeMappingVec3Or(
        addMappingVec3(scaleMappingVec3(base, 1.0f - amount), scaleMappingVec3(target, amount)),
        base
    );
}

float smoothStep(const float edge0, const float edge1, const float value) noexcept
{
    float t = (value - edge0) / (edge1 - edge0);
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    return t * t * (3.0f - 2.0f * t);
}

Vec3 projectedDirection(const Vec3 value, const Vec3 axis) noexcept
{
    return normalizeMappingVec3Or(projectOffAxis(value, axis), Vec3{0.0f, 0.0f, 0.0f});
}

Vec3 limitAngularStep(const Vec3 previous, const Vec3 target, const float folded, const float nearChest) noexcept
{
    const float dot = std::clamp(dotMappingVec3(previous, target), -0.999f, 0.999f);
    const float angle = std::acos(dot);
    const float maxDegrees = std::clamp(26.0f - folded * 15.0f - nearChest * 5.0f, 8.0f, 26.0f);
    const float maxAngle = degreesToRadians(maxDegrees);
    if (angle <= maxAngle) {
        return target;
    }
    const float t = maxAngle / angle;
    const float sinAngle = std::sin(angle);
    if (sinAngle <= 0.0001f) {
        return blendDirection(previous, target, t);
    }
    return normalizeMappingVec3Or(
        addMappingVec3(
            scaleMappingVec3(previous, std::sin((1.0f - t) * angle) / sinAngle),
            scaleMappingVec3(target, std::sin(t * angle) / sinAngle)
        ),
        previous
    );
}

Vec3 stableAgainstPrevious(
    Vec3 poleDir,
    const Vec3 armAxis,
    const Vec3 fallback,
    const Vec3 previous,
    const bool previousValid,
    const float nearChest,
    const float extension,
    const float wristStep,
    const float projectionLength
) noexcept {
    if (!previousValid) {
        return poleDir;
    }
    Vec3 previousDir = projectedDirection(previous, armAxis);
    if (lengthMappingVec3(previousDir) <= 0.0001f) {
        previousDir = fallback;
    }
    const float alignment = dotMappingVec3(previousDir, poleDir);
    const float folded = 1.0f - smoothStep(0.30f, 0.65f, extension);
    const float halfFold = smoothStep(0.42f, 0.62f, extension) *
        (1.0f - smoothStep(0.78f, 0.92f, extension));
    const float fastWrist = smoothStep(0.018f, 0.045f, wristStep);
    const float weakProjection = 1.0f - smoothStep(0.06f, 0.20f, projectionLength);
    const float opposition = 1.0f - smoothStep(-0.35f, 0.15f, alignment);
    float follow = 0.48f - nearChest * 0.15f - folded * 0.30f;
    follow -= weakProjection * 0.32f + opposition * 0.34f;
    follow -= halfFold * fastWrist * 0.22f;
    if (follow < 0.08f) {
        follow = 0.08f;
    } else if (follow > 0.55f) {
        follow = 0.55f;
    }
    const float limitFolded = std::clamp(folded + halfFold * fastWrist * 0.55f, 0.0f, 1.0f);
    return limitAngularStep(previousDir, blendDirection(previousDir, poleDir, follow), limitFolded, nearChest);
}

bool isRestFallback(const MappingCalibrationData& calibration, const MappingTrackerRole role) noexcept
{
    return calibration.targetBindings[static_cast<std::size_t>(slotIndex(role))].source ==
        MappingVirtualTargetSource::RestFallback;
}

bool hasTarget(
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    return targets[static_cast<std::size_t>(slotIndex(role))].valid;
}

bool hasDrivenTarget(
    const MappingCalibrationData& calibration,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    const std::size_t index = static_cast<std::size_t>(slotIndex(role));
    return calibration.targetBindings[index].source != MappingVirtualTargetSource::RestFallback &&
        targets[index].valid;
}

float restDistance(const ProfileSkeletonJoints& rest, const int child, const int parent) noexcept
{
    return distanceMappingVec3(rest[child].positionMeters, rest[parent].positionMeters);
}

MappingTransform makePoleTarget(
    const MappingTransform& chest,
    const Vec3 shoulder,
    const Vec3 wrist,
    const float poleDistance,
    const bool left,
    const Vec3 previousPoleDirection,
    const bool previousPoleDirectionValid,
    const Vec3 previousPoleTarget,
    const bool previousPoleTargetValid
) noexcept {
    const Vec3 chestForward = rotateMappingVector(chest, Vec3{0.0f, 0.0f, 1.0f});
    const Vec3 chestSide = rotateMappingVector(chest, Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 chestUp = rotateMappingVector(chest, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 side = left ? chestSide : scaleMappingVec3(chestSide, -1.0f);
    const Vec3 back = scaleMappingVec3(chestForward, -1.0f);
    const Vec3 armVector = subMappingVec3(wrist, shoulder);
    const Vec3 armAxis = normalizeMappingVec3Or(armVector, back);
    const float extension = poleDistance > 0.0001f
        ? saturate(lengthMappingVec3(armVector) / poleDistance)
        : 1.0f;
    const float nearChest = saturate(1.0f - distanceMappingVec3(wrist, chest.position) / kAntiTorsoDistance);
    const float wristStep = previousPoleTargetValid ? distanceMappingVec3(wrist, previousPoleTarget) : 0.0f;
    const Vec3 preferred = sampleElbowPosePreferredDirection(chest, wrist, left);
    const Vec3 antiTorso = normalizeMappingVec3Or(
        addMappingVec3(
            addMappingVec3(scaleMappingVec3(side, 0.60f), scaleMappingVec3(back, 0.35f)),
            scaleMappingVec3(chestUp, -0.25f)
        ),
        preferred
    );
    const Vec3 desired = blendDirection(preferred, antiTorso, nearChest * kAntiTorsoBlend);
    const Vec3 projected = projectOffAxis(desired, armAxis);
    const float projectionLength = lengthMappingVec3(projected);
    Vec3 fallback = projectedDirection(side, armAxis);
    if (lengthMappingVec3(fallback) <= 0.0001f) {
        fallback = projectedDirection(back, armAxis);
    }
    Vec3 poleDir = normalizeMappingVec3Or(projected, fallback);
    const Vec3 sidePole = projectedDirection(side, armAxis);
    if (lengthMappingVec3(sidePole) > 0.0001f) {
        const float sideNeed = saturate((nearChest * 0.55f - dotMappingVec3(poleDir, sidePole)) / 0.70f);
        poleDir = blendDirection(poleDir, sidePole, sideNeed * (0.18f + nearChest * 0.32f));
    }
    poleDir = stableAgainstPrevious(
        poleDir,
        armAxis,
        fallback,
        previousPoleDirection,
        previousPoleDirectionValid,
        nearChest,
        extension,
        wristStep,
        projectionLength
    );
    return {
        addMappingVec3(shoulder, scaleMappingVec3(poleDir, poleDistance)),
        chest.rotation
    };
}

void applyOneArm(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& solvedCore,
    std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const std::array<Vec3, kMappingPoleCount>& previousPoleDirections,
    const std::array<bool, kMappingPoleCount>& previousPoleDirectionValid,
    const std::array<Vec3, kMappingPoleCount>& previousPoleTargets,
    const std::array<bool, kMappingPoleCount>& previousPoleTargetValid,
    const bool left
) {
    const MappingTrackerRole armRole = left ? MappingTrackerRole::LeftArm : MappingTrackerRole::RightArm;
    const MappingTrackerRole handRole = left ? MappingTrackerRole::LeftHand : MappingTrackerRole::RightHand;
    if (!isRestFallback(calibration, armRole) ||
        !hasDrivenTarget(calibration, targets, handRole) ||
        !hasTarget(targets, MappingTrackerRole::Chest)) {
        return;
    }
    const int shoulder = left ? kProfileJointLeftShoulder : kProfileJointRightShoulder;
    const int elbow = left ? kProfileJointLeftArm : kProfileJointRightArm;
    const int wrist = left ? kProfileJointLeftForeArm : kProfileJointRightForeArm;
    const int poleIndex = mappingPoleIndex(left ? MappingPoleKind::LeftArm : MappingPoleKind::RightArm);
    const float distance = restDistance(rest, elbow, shoulder) + restDistance(rest, wrist, elbow);
    const MappingTransform pole = makePoleTarget(
        targets[static_cast<std::size_t>(slotIndex(MappingTrackerRole::Chest))].transform,
        solvedCore[shoulder].positionMeters,
        targets[static_cast<std::size_t>(slotIndex(handRole))].transform.position,
        distance,
        left,
        previousPoleDirections[static_cast<std::size_t>(poleIndex)],
        previousPoleDirectionValid[static_cast<std::size_t>(poleIndex)],
        previousPoleTargets[static_cast<std::size_t>(poleIndex)],
        previousPoleTargetValid[static_cast<std::size_t>(poleIndex)]
    );
    const std::size_t armIndex = static_cast<std::size_t>(slotIndex(armRole));
    targets[armIndex] = MappingVirtualTarget{armRole, pole, pole, true};
}

} // namespace

void applyEstimatedArmPoleTargets(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    const ProfileSkeletonJoints& solvedCore,
    const std::array<Vec3, kMappingPoleCount>& previousPoleDirections,
    const std::array<bool, kMappingPoleCount>& previousPoleDirectionValid,
    const std::array<Vec3, kMappingPoleCount>& previousPoleTargets,
    const std::array<bool, kMappingPoleCount>& previousPoleTargetValid,
    std::array<MappingVirtualTarget, kMappingSlotCount>& targets
) {
    applyOneArm(
        calibration,
        rest,
        solvedCore,
        targets,
        previousPoleDirections,
        previousPoleDirectionValid,
        previousPoleTargets,
        previousPoleTargetValid,
        true
    );
    applyOneArm(
        calibration,
        rest,
        solvedCore,
        targets,
        previousPoleDirections,
        previousPoleDirectionValid,
        previousPoleTargets,
        previousPoleTargetValid,
        false
    );
}

} // namespace ovtr::win32
