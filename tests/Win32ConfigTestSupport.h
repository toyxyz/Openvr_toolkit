#pragma once

#include <cmath>

namespace ovtr::test {

inline bool win32ConfigNearlyEqual(const float lhs, const float rhs)
{
    return std::fabs(lhs - rhs) < 0.0001f;
}

} // namespace ovtr::test
