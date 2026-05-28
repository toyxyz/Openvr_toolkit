#pragma once

namespace ovtr::win32 {

inline constexpr float kViewportMathPi = 3.14159265359f;

inline float degreesToRadians(const float degrees) noexcept
{
    return degrees * kViewportMathPi / 180.0f;
}

} // namespace ovtr::win32
