#include "platform/win32/VmcFingerRotationFilter.h"

#include "math/PoseTransform.h"
#include "math/QuaternionUtils.h"

#include <cmath>

namespace ovtr::win32 {
namespace {

float axisLength(const std::array<float, 3>& axis) noexcept
{
    return std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
}

float axisDot(const std::array<float, 3>& left, const std::array<float, 3>& right) noexcept
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

std::array<float, 3> axisCross(const std::array<float, 3>& left, const std::array<float, 3>& right) noexcept
{
    return {
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0]
    };
}

std::array<float, 3> normalizeAxisOrDefault(const std::array<float, 3>& axis) noexcept
{
    const float length = axisLength(axis);
    if (length <= 0.00001f) {
        return {0.0f, 0.0f, 1.0f};
    }
    return {axis[0] / length, axis[1] / length, axis[2] / length};
}

std::array<float, 4> shortestArcRotation(
    const std::array<float, 3>& from,
    const std::array<float, 3>& to
) noexcept {
    const std::array<float, 3> a = normalizeAxisOrDefault(from);
    const std::array<float, 3> b = normalizeAxisOrDefault(to);
    const float dot = axisDot(a, b);
    if (dot > 0.9999f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    if (dot < -0.9999f) {
        const std::array<float, 3> fallback =
            std::fabs(a[1]) < 0.9f ? std::array<float, 3>{0.0f, 1.0f, 0.0f} : std::array<float, 3>{1.0f, 0.0f, 0.0f};
        const std::array<float, 3> turnAxis = normalizeAxisOrDefault(axisCross(fallback, a));
        return {turnAxis[0], turnAxis[1], turnAxis[2], 0.0f};
    }
    const std::array<float, 3> turnAxis = axisCross(a, b);
    return ovtr::normalizeQuaternion({turnAxis[0], turnAxis[1], turnAxis[2], 1.0f + dot});
}

} // namespace

std::array<float, 4> removeVmcFingerLocalRoll(
    const std::array<float, 4>& localRotation,
    const std::array<float, 3>& childAxis
) noexcept {
    const std::array<float, 4> q = ovtr::normalizeQuaternion(localRotation);
    const std::array<float, 3> axis = normalizeAxisOrDefault(childAxis);
    const std::array<float, 3> rotatedAxis = ovtr::rotatePositionByQuaternion(q, axis);
    return shortestArcRotation(axis, rotatedAxis);
}

} // namespace ovtr::win32
