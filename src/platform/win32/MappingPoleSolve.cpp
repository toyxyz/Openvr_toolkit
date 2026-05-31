#include "platform/win32/MappingPoleSolve.h"

#include "platform/win32/MappingTransformMath.h"

#include <algorithm>
#include <cmath>

namespace ovtr::win32 {
namespace {

Vec3 projectedDirection(const Vec3 axis, const Vec3 candidate) noexcept
{
    return subMappingVec3(candidate, scaleMappingVec3(axis, dotMappingVec3(candidate, axis)));
}

Vec3 normalizedProjectedOrZero(const Vec3 axis, const Vec3 candidate) noexcept
{
    return normalizeMappingVec3Or(projectedDirection(axis, candidate), Vec3{0.0f, 0.0f, 0.0f});
}

Vec3 fallbackDirection(const Vec3 axis, const Vec3 restDirection) noexcept
{
    Vec3 direction = normalizedProjectedOrZero(axis, restDirection);
    if (lengthMappingVec3(direction) > 0.0f) {
        return direction;
    }
    const Vec3 up = std::fabs(axis.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
    direction = normalizedProjectedOrZero(axis, up);
    if (lengthMappingVec3(direction) > 0.0f) {
        return direction;
    }
    return Vec3{0.0f, 0.0f, 1.0f};
}

bool isReachLimited(const MappingPoleSolveInput& input) noexcept
{
    const float distance = distanceMappingVec3(input.root, input.target);
    const float minDistance = std::fabs(input.upperLength - input.lowerLength) + 0.0001f;
    const float maxDistance = input.upperLength + input.lowerLength - 0.0001f;
    return distance < minDistance || distance > std::max(minDistance, maxDistance);
}

} // namespace

int mappingPoleIndex(const MappingPoleKind kind) noexcept
{
    return static_cast<int>(kind);
}

MappingPoleSolveResult solveMappingPole(const MappingPoleSolveInput& input) noexcept
{
    MappingPoleSolveResult result;
    const Vec3 targetVector = subMappingVec3(input.target, input.root);
    const Vec3 restDirection = subMappingVec3(input.restMid, input.root);
    const Vec3 axis = normalizeMappingVec3Or(
        targetVector,
        normalizeMappingVec3Or(restDirection, Vec3{1.0f, 0.0f, 0.0f})
    );
    result.direction = normalizedProjectedOrZero(axis, subMappingVec3(input.hint, input.root));
    if (lengthMappingVec3(result.direction) <= 0.0001f) {
        result.fallback = true;
        result.direction = input.previousDirectionValid
            ? input.previousDirection
            : fallbackDirection(axis, restDirection);
    }
    result.direction = normalizeMappingVec3Or(result.direction, fallbackDirection(axis, restDirection));
    result.limited = isReachLimited(input);
    result.polePoint = addMappingVec3(input.root, scaleMappingVec3(result.direction, 0.25f));
    result.debug = MappingDebugPole{input.root, input.target, result.polePoint, true, result.fallback, result.limited};
    return result;
}

} // namespace ovtr::win32
