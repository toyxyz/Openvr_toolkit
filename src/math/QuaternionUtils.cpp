#include "math/QuaternionUtils.h"

#include <cmath>

namespace ovtr {

float quaternionLengthSquared(const std::array<float, 4>& q)
{
    return q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
}

std::array<float, 4> normalizeQuaternion(const std::array<float, 4>& q)
{
    const float lengthSquared = quaternionLengthSquared(q);
    if (lengthSquared <= 0.0f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return {q[0] * inverseLength, q[1] * inverseLength, q[2] * inverseLength, q[3] * inverseLength};
}

bool isNearlyUnitQuaternion(const std::array<float, 4>& q, const float epsilon)
{
    return std::fabs(quaternionLengthSquared(q) - 1.0f) <= epsilon;
}

} // namespace ovtr

