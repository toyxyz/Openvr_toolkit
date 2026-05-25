#pragma once

#include <array>

namespace ovtr {

float quaternionLengthSquared(const std::array<float, 4>& q);
std::array<float, 4> normalizeQuaternion(const std::array<float, 4>& q);
bool isNearlyUnitQuaternion(const std::array<float, 4>& q, float epsilon = 0.0001f);

} // namespace ovtr

