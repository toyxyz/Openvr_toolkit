#pragma once

#include "platform/win32/MappingModel.h"
#include "platform/win32/ViewportMath.h"

namespace ovtr::win32 {

struct TwoBoneIkResult {
    Vec3 mid{};
    Vec3 end{};
};

TwoBoneIkResult solveTwoBoneIk(
    Vec3 root,
    Vec3 target,
    Vec3 pole,
    float upperLength,
    float lowerLength,
    Vec3 restMid,
    float softIkStrength = kDefaultMappingSoftIkStrength
) noexcept;

} // namespace ovtr::win32
