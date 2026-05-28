#include "platform/win32/ViewportCamera.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

CameraView cameraViewFromState(const AppWindowState& state)
{
    return cameraViewFromState(static_cast<const AppViewportState&>(state));
}

float cameraDepthForWorldPoint(const AppWindowState& state, const Vec3 point)
{
    return cameraDepthForWorldPoint(static_cast<const AppViewportState&>(state), point);
}

void applyScreenSpacePan(AppWindowState& state, const int dx, const int dy)
{
    applyScreenSpacePan(static_cast<AppViewportState&>(state), dx, dy);
}

void applyCameraDolly(AppWindowState& state, const float distance)
{
    applyCameraDolly(static_cast<AppViewportState&>(state), distance);
}

} // namespace ovtr::win32
