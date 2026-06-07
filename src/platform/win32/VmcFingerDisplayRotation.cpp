#include "platform/win32/VmcFingerDisplayRotation.h"

#include "math/QuaternionUtils.h"

#include <cmath>

namespace ovtr::win32 {
namespace {

std::array<float, 3> scaleArray3(const std::array<float, 3>& value, const float scale) noexcept
{
    return {value[0] * scale, value[1] * scale, value[2] * scale};
}

std::array<float, 3> subArray3(const std::array<float, 3>& left, const std::array<float, 3>& right) noexcept
{
    return {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
}

float dotArray3(const std::array<float, 3>& left, const std::array<float, 3>& right) noexcept
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

std::array<float, 3> crossArray3(const std::array<float, 3>& left, const std::array<float, 3>& right) noexcept
{
    return {
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0]
    };
}

float lengthArray3(const std::array<float, 3>& value) noexcept
{
    return std::sqrt(dotArray3(value, value));
}

std::array<float, 3> normalizeArray3Or(
    const std::array<float, 3>& value,
    const std::array<float, 3>& fallback
) noexcept {
    const float length = lengthArray3(value);
    return length > 0.00001f ? scaleArray3(value, 1.0f / length) : fallback;
}

std::array<float, 3> fallbackSideForDirection(const std::array<float, 3>& y) noexcept
{
    const std::array<float, 3> hint = std::fabs(y[1]) < 0.9f
        ? std::array<float, 3>{0.0f, 1.0f, 0.0f}
        : std::array<float, 3>{1.0f, 0.0f, 0.0f};
    return normalizeArray3Or(subArray3(hint, scaleArray3(y, dotArray3(hint, y))), {1.0f, 0.0f, 0.0f});
}

std::array<float, 4> quaternionFromBasis(
    const std::array<float, 3>& x,
    const std::array<float, 3>& y,
    const std::array<float, 3>& z
) noexcept {
    const float trace = x[0] + y[1] + z[2];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return ovtr::normalizeQuaternion({(y[2] - z[1]) / s, (z[0] - x[2]) / s, (x[1] - y[0]) / s, 0.25f * s});
    }
    if (x[0] > y[1] && x[0] > z[2]) {
        const float s = std::sqrt(1.0f + x[0] - y[1] - z[2]) * 2.0f;
        return ovtr::normalizeQuaternion({0.25f * s, (y[0] + x[1]) / s, (z[0] + x[2]) / s, (y[2] - z[1]) / s});
    }
    if (y[1] > z[2]) {
        const float s = std::sqrt(1.0f + y[1] - x[0] - z[2]) * 2.0f;
        return ovtr::normalizeQuaternion({(y[0] + x[1]) / s, 0.25f * s, (z[1] + y[2]) / s, (z[0] - x[2]) / s});
    }
    const float s = std::sqrt(1.0f + z[2] - x[0] - y[1]) * 2.0f;
    return ovtr::normalizeQuaternion({(z[0] + x[2]) / s, (z[1] + y[2]) / s, 0.25f * s, (x[1] - y[0]) / s});
}

} // namespace

std::array<float, 4> vmcFingerDisplayRotationForSegment(
    const std::array<float, 3>& parent,
    const std::array<float, 3>& child,
    const std::array<float, 3>& palmSide
) noexcept {
    const std::array<float, 3> y = normalizeArray3Or(subArray3(child, parent), {0.0f, 1.0f, 0.0f});
    const std::array<float, 3> fallbackX = fallbackSideForDirection(y);
    const std::array<float, 3> projectedPalm = subArray3(palmSide, scaleArray3(y, dotArray3(palmSide, y)));
    const std::array<float, 3> x = normalizeArray3Or(projectedPalm, fallbackX);
    const std::array<float, 3> z = normalizeArray3Or(crossArray3(x, y), {0.0f, 0.0f, 1.0f});
    return quaternionFromBasis(x, y, z);
}

} // namespace ovtr::win32
