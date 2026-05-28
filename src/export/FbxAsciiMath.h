#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace ovtr {

using FbxMatrix3 = std::array<double, 9>;

std::int64_t fbxSecondsToTicks(double seconds);
std::array<double, 3> fbxEulerDegreesToRadians(const std::array<double, 3>& eulerDegrees);
std::array<double, 3> fbxEulerRadiansToDegrees(const std::array<double, 3>& eulerRadians);
FbxMatrix3 fbxQuaternionToMatrix3(const std::array<float, 4>& quaternion);
std::array<double, 3> fbxMatrix3ToEulerXyzRadians(const FbxMatrix3& matrix);
std::array<double, 3> fbxCompatibleEulerXyzRadiansFromMatrix(
    const FbxMatrix3& matrix,
    const std::array<double, 3>& previous
);
void unwrapFbxEulerRadians(std::vector<std::array<double, 3>>& eulers);

} // namespace ovtr
