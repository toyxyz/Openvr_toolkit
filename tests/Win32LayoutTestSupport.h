#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cmath>

namespace ovtr::test {

inline bool sameRect(const RECT& rect, const long left, const long top, const long right, const long bottom)
{
    return rect.left == left &&
        rect.top == top &&
        rect.right == right &&
        rect.bottom == bottom;
}

inline bool nearFloat(const float actual, const float expected)
{
    return std::fabs(actual - expected) < 0.00001f;
}

} // namespace ovtr::test
