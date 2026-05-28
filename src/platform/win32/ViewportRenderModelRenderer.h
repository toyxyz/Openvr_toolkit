#pragma once

#include "data/SessionTypes.h"

namespace ovtr::win32 {

struct AppViewportState;
struct CameraView;

bool drawSteamVRRenderModel3D(
    AppViewportState& state,
    const ovtr::PoseSample& pose,
    const ovtr::DeviceDescriptor* device,
    int viewportHeight,
    bool selected,
    const CameraView& cameraView,
    float outlineWorldUnitsPerPixel
);

} // namespace ovtr::win32
