#pragma once

#include "data/SessionTypes.h"

namespace ovtr::win32 {

struct AppDeviceState;
struct AppOriginState;
struct AppRuntimeState;
struct AppViewportState;
struct AppWindowState;
struct CameraView;

void drawTrackedDevices3D(
    AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const AppOriginState& originState,
    AppViewportState& viewportState,
    int viewportHeight,
    const CameraView& cameraView,
    float outlineWorldUnitsPerPixel
);
void drawTrackedDevices3D(AppWindowState& state, int viewportHeight);
void drawTrackedDeviceLabels3D(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const AppOriginState& originState,
    const AppViewportState& viewportState
);
void drawTrackedDeviceLabels3D(const AppWindowState& state);

} // namespace ovtr::win32
