#pragma once

#include <array>

namespace ovtr::win32 {

std::array<float, 4> vmcFingerDisplayRotationForSegment(
    const std::array<float, 3>& parent,
    const std::array<float, 3>& child,
    const std::array<float, 3>& palmSide
) noexcept;

} // namespace ovtr::win32
