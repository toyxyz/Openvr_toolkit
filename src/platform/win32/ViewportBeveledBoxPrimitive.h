#pragma once

#include "platform/win32/ViewportMath.h"

namespace ovtr::win32 {

void drawBeveledSegmentBox3D(
    Vec3 start,
    Vec3 end,
    float halfSide,
    float halfDepth
);

void drawBeveledSegmentBoxWithSideHint3D(
    Vec3 start,
    Vec3 end,
    Vec3 sideHint,
    float halfSide,
    float halfDepth
);

} // namespace ovtr::win32
