#include "export/FbxAsciiMath.h"

#include "math/QuaternionUtils.h"

#include <cmath>

namespace ovtr {
namespace {

constexpr std::int64_t kFbxTicksPerSecond = 46'186'158'000LL;
constexpr double kDegreesToRadians = 0.017453292519943295769;
constexpr double kRadiansToDegrees = 57.295779513082320876;

} // namespace

std::int64_t fbxSecondsToTicks(const double seconds)
{
    return static_cast<std::int64_t>(std::llround(seconds * static_cast<double>(kFbxTicksPerSecond)));
}

std::array<double, 3> fbxEulerDegreesToRadians(const std::array<double, 3>& eulerDegrees)
{
    return {
        eulerDegrees[0] * kDegreesToRadians,
        eulerDegrees[1] * kDegreesToRadians,
        eulerDegrees[2] * kDegreesToRadians,
    };
}

std::array<double, 3> fbxEulerRadiansToDegrees(const std::array<double, 3>& eulerRadians)
{
    return {
        eulerRadians[0] * kRadiansToDegrees,
        eulerRadians[1] * kRadiansToDegrees,
        eulerRadians[2] * kRadiansToDegrees,
    };
}

FbxMatrix3 fbxQuaternionToMatrix3(const std::array<float, 4>& quaternion)
{
    const std::array<float, 4> q = normalizeQuaternion(quaternion);
    const double x = q[0];
    const double y = q[1];
    const double z = q[2];
    const double w = q[3];

    return {
        1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - z * w), 2.0 * (x * z + y * w),
        2.0 * (x * y + z * w), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - x * w),
        2.0 * (x * z - y * w), 2.0 * (y * z + x * w), 1.0 - 2.0 * (x * x + y * y),
    };
}

} // namespace ovtr
