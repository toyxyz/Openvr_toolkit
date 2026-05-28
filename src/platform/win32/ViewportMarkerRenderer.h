#pragma once

#include "platform/win32/AppMarkerState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/ViewportMath.h"

namespace ovtr::win32 {

void drawSceneMarkers3D(
    const AppMarkerState& markerState,
    AppViewportState& viewportState,
    int viewportHeight,
    const CameraView& cameraView,
    float outlineWorldUnitsPerPixel
);

} // namespace ovtr::win32
