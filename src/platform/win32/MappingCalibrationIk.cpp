#include "platform/win32/MappingCalibrationIk.h"

#include "platform/win32/MappingTransformMath.h"

#include <algorithm>
#include <cmath>

namespace ovtr::win32 {
namespace {

Vec3 projectedPoleDirection(const Vec3 axis, const Vec3 candidate) noexcept
{
    const Vec3 projected = subMappingVec3(candidate, scaleMappingVec3(axis, dotMappingVec3(candidate, axis)));
    return normalizeMappingVec3Or(projected, Vec3{0.0f, 0.0f, 0.0f});
}

Vec3 fallbackPoleDirection(const Vec3 axis, const Vec3 restDirection) noexcept
{
    Vec3 direction = projectedPoleDirection(axis, restDirection);
    if (lengthMappingVec3(direction) > 0.0f) {
        return direction;
    }
    const Vec3 up = std::fabs(axis.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
    direction = projectedPoleDirection(axis, up);
    if (lengthMappingVec3(direction) > 0.0f) {
        return direction;
    }
    return Vec3{0.0f, 0.0f, 1.0f};
}

float softenedIkDistance(
    const float rawDistance,
    const float minDistance,
    const float maxDistance,
    const float softIkStrength
) noexcept {
    const float range = maxDistance - minDistance;
    if (range <= 0.0001f) {
        return maxDistance;
    }
    if (softIkStrength <= 0.0f) {
        return std::clamp(rawDistance, minDistance, maxDistance);
    }
    const float softness = std::min(range * 0.5f, std::max(0.0001f, maxDistance * softIkStrength));
    const float softStart = maxDistance - softness;
    if (rawDistance <= softStart) {
        return std::clamp(rawDistance, minDistance, maxDistance);
    }
    const float extra = rawDistance - softStart;
    const float softened = softStart + softness * (1.0f - std::exp(-extra / softness));
    const float safeMax = std::max(minDistance, maxDistance - 0.0001f);
    return std::clamp(softened, minDistance, safeMax);
}

} // namespace

TwoBoneIkResult solveTwoBoneIk(
    const Vec3 root,
    const Vec3 target,
    const Vec3 pole,
    const float upperLength,
    const float lowerLength,
    const Vec3 restMid,
    const float softIkStrength
) noexcept {
    if (upperLength <= 0.0f || lowerLength <= 0.0f) {
        return TwoBoneIkResult{restMid, target};
    }

    const Vec3 targetVector = subMappingVec3(target, root);
    const Vec3 restDirection = subMappingVec3(restMid, root);
    const Vec3 axis = normalizeMappingVec3Or(targetVector, normalizeMappingVec3Or(restDirection, Vec3{1.0f, 0.0f, 0.0f}));
    const float rawDistance = lengthMappingVec3(targetVector);
    const float minDistance = std::max(0.0001f, std::fabs(upperLength - lowerLength) + 0.0001f);
    const float maxDistance = std::max(minDistance, upperLength + lowerLength);
    const float distance = softenedIkDistance(rawDistance, minDistance, maxDistance, softIkStrength);
    const Vec3 end = addMappingVec3(root, scaleMappingVec3(axis, distance));

    const float along = (upperLength * upperLength - lowerLength * lowerLength + distance * distance) /
        (2.0f * distance);
    const float sideSquared = std::max(0.0f, upperLength * upperLength - along * along);
    Vec3 poleDirection = projectedPoleDirection(axis, subMappingVec3(pole, root));
    if (lengthMappingVec3(poleDirection) <= 0.0f) {
        poleDirection = fallbackPoleDirection(axis, restDirection);
    }

    const Vec3 mid = addMappingVec3(
        addMappingVec3(root, scaleMappingVec3(axis, along)),
        scaleMappingVec3(poleDirection, std::sqrt(sideSquared))
    );
    return TwoBoneIkResult{mid, end};
}

} // namespace ovtr::win32
