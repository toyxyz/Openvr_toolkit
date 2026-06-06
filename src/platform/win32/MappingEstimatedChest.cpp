#include "platform/win32/MappingEstimatedChest.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/MappingTransformMath.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace ovtr::win32 {
namespace {

constexpr float kDefaultChestRatio = 0.6f;
constexpr float kHeadYawWeight = 0.3f;
constexpr float kMaxLeanDegrees = 30.0f;
constexpr float kPi = 3.14159265358979323846f;

int slotIndex(const MappingTrackerRole role) noexcept
{
    return static_cast<int>(mappingSlotForRole(role));
}

Vec3 cross(const Vec3 a, const Vec3 b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 lerp(const Vec3 a, const Vec3 b, const float t) noexcept
{
    return addMappingVec3(scaleMappingVec3(a, 1.0f - t), scaleMappingVec3(b, t));
}

Vec3 projectOffAxis(const Vec3 value, const Vec3 axis) noexcept
{
    return subMappingVec3(value, scaleMappingVec3(axis, dotMappingVec3(value, axis)));
}

Vec3 yawForward(const std::array<float, 4>& rotation) noexcept
{
    const std::array<float, 3> forward = rotatePositionByQuaternion(rotation, {0.0f, 0.0f, 1.0f});
    return normalizeMappingVec3Or({forward[0], 0.0f, forward[2]}, {0.0f, 0.0f, 1.0f});
}

Vec3 clampedChestUp(const Vec3 rawUp) noexcept
{
    const Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const Vec3 up = normalizeMappingVec3Or(rawUp, worldUp);
    const float maxLean = kMaxLeanDegrees * kPi / 180.0f;
    const float minDot = std::cos(maxLean);
    const float d = std::clamp(dotMappingVec3(up, worldUp), -1.0f, 1.0f);
    if (d >= minDot) {
        return up;
    }
    const Vec3 horizontal = normalizeMappingVec3Or(projectOffAxis(up, worldUp), {0.0f, 0.0f, 1.0f});
    return normalizeMappingVec3Or(
        addMappingVec3(scaleMappingVec3(worldUp, minDot), scaleMappingVec3(horizontal, std::sin(maxLean))),
        worldUp
    );
}

std::array<float, 4> quaternionFromBasis(const Vec3 side, const Vec3 up, const Vec3 forward) noexcept
{
    const float m00 = side.x;
    const float m01 = up.x;
    const float m02 = forward.x;
    const float m10 = side.y;
    const float m11 = up.y;
    const float m12 = forward.y;
    const float m20 = side.z;
    const float m21 = up.z;
    const float m22 = forward.z;
    const float trace = m00 + m11 + m22;
    std::array<float, 4> q{};
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25f * s};
    } else if (m00 > m11 && m00 > m22) {
        const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
        q = {0.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    } else if (m11 > m22) {
        const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
        q = {(m01 + m10) / s, 0.25f * s, (m12 + m21) / s, (m02 - m20) / s};
    } else {
        const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
        q = {(m02 + m20) / s, (m12 + m21) / s, 0.25f * s, (m10 - m01) / s};
    }
    return normalizeQuaternion(q);
}

float chestRatioFromRest(const ProfileSkeletonJoints& rest) noexcept
{
    const float torso = distanceMappingVec3(
        rest[kProfileJointNeck].positionMeters,
        rest[kProfileJointHips].positionMeters
    );
    if (torso <= 0.0001f) {
        return kDefaultChestRatio;
    }
    const float chest = distanceMappingVec3(
        rest[kProfileJointSpine2].positionMeters,
        rest[kProfileJointHips].positionMeters
    );
    return std::clamp(chest / torso, 0.3f, 0.8f);
}

MappingTransform estimateChest(
    const ProfileSkeletonJoints& rest,
    const MappingTransform& head,
    const MappingTransform& pelvis
) noexcept {
    const Vec3 neckFromHead = subMappingVec3(
        rest[kProfileJointNeck].positionMeters,
        rest[kProfileJointHead].positionMeters
    );
    const Vec3 neck = transformMappingPoint(head, neckFromHead);
    const Vec3 spine = subMappingVec3(neck, pelvis.position);
    const Vec3 up = clampedChestUp(spine);
    Vec3 forward = normalizeMappingVec3Or(
        projectOffAxis(lerp(yawForward(pelvis.rotation), yawForward(head.rotation), kHeadYawWeight), up),
        projectOffAxis(yawForward(pelvis.rotation), up)
    );
    const Vec3 side = normalizeMappingVec3Or(cross(up, forward), {1.0f, 0.0f, 0.0f});
    forward = normalizeMappingVec3Or(cross(side, up), forward);
    return {
        addMappingVec3(pelvis.position, scaleMappingVec3(spine, chestRatioFromRest(rest))),
        quaternionFromBasis(side, up, forward)
    };
}

bool isRestFallback(const MappingCalibrationData& calibration, const MappingTrackerRole role) noexcept
{
    return calibration.targetBindings[static_cast<std::size_t>(slotIndex(role))].source ==
        MappingVirtualTargetSource::RestFallback;
}

bool hasMeasuredTarget(
    const MappingCalibrationData& calibration,
    const std::array<MappingVirtualTarget, kMappingSlotCount>& targets,
    const MappingTrackerRole role
) noexcept {
    const std::size_t index = static_cast<std::size_t>(slotIndex(role));
    return calibration.targetBindings[index].source != MappingVirtualTargetSource::RestFallback &&
        targets[index].valid;
}

} // namespace

void applyEstimatedChestTarget(
    const MappingCalibrationData& calibration,
    const ProfileSkeletonJoints& rest,
    std::array<MappingVirtualTarget, kMappingSlotCount>& targets
) {
    if (!isRestFallback(calibration, MappingTrackerRole::Chest) ||
        !hasMeasuredTarget(calibration, targets, MappingTrackerRole::Head) ||
        !hasMeasuredTarget(calibration, targets, MappingTrackerRole::Pelvis)) {
        return;
    }
    const std::size_t chestIndex = static_cast<std::size_t>(slotIndex(MappingTrackerRole::Chest));
    const MappingTransform chest = estimateChest(
        rest,
        targets[static_cast<std::size_t>(slotIndex(MappingTrackerRole::Head))].transform,
        targets[static_cast<std::size_t>(slotIndex(MappingTrackerRole::Pelvis))].transform
    );
    targets[chestIndex] = MappingVirtualTarget{MappingTrackerRole::Chest, chest, chest, true};
}

} // namespace ovtr::win32
