#pragma once

#include "data/SessionTypes.h"

namespace ovtr::win32 {

struct AppViewportState;

bool drawSteamVRRenderModel3D(
    AppViewportState& state,
    const ovtr::PoseSample& pose,
    const ovtr::DeviceDescriptor* device,
    int viewportHeight,
    bool selected
);

} // namespace ovtr::win32
