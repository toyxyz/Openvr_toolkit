#pragma once

#include <array>

namespace ovtr::win32 {

std::array<float, 4> removeVmcFingerLocalRoll(
    const std::array<float, 4>& localRotation,
    const std::array<float, 3>& childAxis
) noexcept;

} // namespace ovtr::win32
