#include "platform/win32/ViewportCamera.h"

#include "platform/win32/AppViewportState.h"

namespace ovtr::win32 {
namespace {

constexpr float kMinimumCameraDistance = 0.08f;
constexpr float kViewportFovDegrees = 48.0f;
constexpr float kRenderModelOutlinePixels = 2.6f;

void applyPanOffset(AppViewportState& state, const Vec3 offset)
{
    state.cameraPanX += offset.x;
    state.cameraPanY += offset.y;
    state.cameraPanZ += offset.z;
}

} // namespace

CameraView cameraViewFromState(const AppViewportState& state)
{
    return {
        state.cameraYawDegrees,
        state.cameraPitchDegrees,
        state.cameraDistance,
        {state.cameraPanX, state.cameraPanY, state.cameraPanZ},
    };
}

void applyScreenSpacePan(AppViewportState& state, const int dx, const int dy)
{
    applyPanOffset(
        state,
        screenSpacePanOffset(cameraViewFromState(state), dx, dy, kMinimumCameraDistance)
    );
}

void applyCameraDolly(AppViewportState& state, const float distance)
{
    applyPanOffset(
        state,
        cameraDollyOffset(cameraViewFromState(state), distance)
    );
}

float cameraDepthForWorldPoint(const CameraView& view, const Vec3 point)
{
    return ovtr::win32::cameraDepthForWorldPoint(
        view,
        point,
        kMinimumCameraDistance
    );
}

float cameraDepthForWorldPoint(const AppViewportState& state, const Vec3 point)
{
    return cameraDepthForWorldPoint(cameraViewFromState(state), point);
}

float outlineExpansionForDepth(const float cameraDepth, const int viewportHeight, const float multiplier)
{
    return ovtr::win32::outlineExpansionForDepth(
        cameraDepth,
        viewportHeight,
        kViewportFovDegrees,
        kRenderModelOutlinePixels,
        multiplier
    );
}

float outlineExpansionForOrtho(const float worldUnitsPerPixel, const float multiplier)
{
    return worldUnitsPerPixel * kRenderModelOutlinePixels * multiplier;
}

} // namespace ovtr::win32
