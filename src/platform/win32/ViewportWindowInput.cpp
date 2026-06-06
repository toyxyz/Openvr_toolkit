#include "platform/win32/ViewportWindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportQuadView.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <cmath>

namespace ovtr::win32 {
namespace {

constexpr float kMinimumCameraDistance = 0.08f;
constexpr float kViewportFovDegrees = 48.0f;
constexpr float kPi = 3.14159265358979323846f;

ViewportPaneKind paneFromClientPoint(HWND hwnd, const AppViewportState& state, const int x, const int y)
{
    if (!state.quadViewEnabled) {
        return ViewportPaneKind::Perspective;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    const QuadViewLayout layout = quadViewLayoutForViewport(rect.right - rect.left, rect.bottom - rect.top);
    return quadViewPaneFromPoint(layout, x, y);
}

float orthoZoomForPane(const AppViewportState& state, const ViewportPaneKind pane) noexcept
{
    switch (pane) {
    case ViewportPaneKind::Front:
        return clampOrthoViewZoom(state.frontViewZoom);
    case ViewportPaneKind::Top:
        return clampOrthoViewZoom(state.topViewZoom);
    case ViewportPaneKind::Left:
        return clampOrthoViewZoom(state.leftViewZoom);
    case ViewportPaneKind::Perspective:
    case ViewportPaneKind::None:
    default:
        return kDefaultOrthoViewZoom;
    }
}

void setOrthoZoomForPane(AppViewportState& state, const ViewportPaneKind pane, const float zoom) noexcept
{
    switch (pane) {
    case ViewportPaneKind::Front:
        state.frontViewZoom = clampOrthoViewZoom(zoom);
        break;
    case ViewportPaneKind::Top:
        state.topViewZoom = clampOrthoViewZoom(zoom);
        break;
    case ViewportPaneKind::Left:
        state.leftViewZoom = clampOrthoViewZoom(zoom);
        break;
    case ViewportPaneKind::Perspective:
    case ViewportPaneKind::None:
    default:
        break;
    }
}

float orthoWorldUnitsPerPixel(HWND hwnd, const AppViewportState& state, const ViewportPaneKind pane)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    const QuadViewLayout layout = quadViewLayoutForViewport(rect.right - rect.left, rect.bottom - rect.top);
    const RECT paneRect = rectForQuadViewPane(layout, pane);
    const int paneHeight = paneRect.bottom - paneRect.top;
    if (paneHeight <= 0) {
        return 0.0f;
    }

    const float distance = positiveCameraDistance(kDefaultCameraDistance, kMinimumCameraDistance);
    const float halfHeight = distance *
        std::tan(kViewportFovDegrees * kPi / 360.0f) /
        orthoZoomForPane(state, pane);
    return (2.0f * halfHeight) / static_cast<float>(paneHeight);
}

void applyOrthoPan(AppViewportState& state, const ViewportPaneKind pane, const Vec3 offset)
{
    switch (pane) {
    case ViewportPaneKind::Front:
        state.frontViewPanX += offset.x;
        state.frontViewPanY += offset.y;
        break;
    case ViewportPaneKind::Top:
        state.topViewPanX += offset.x;
        state.topViewPanZ += offset.z;
        break;
    case ViewportPaneKind::Left:
        state.leftViewPanZ += offset.z;
        state.leftViewPanY += offset.y;
        break;
    case ViewportPaneKind::Perspective:
    case ViewportPaneKind::None:
    default:
        break;
    }
}

void invalidateImportedAnimationControls(HWND viewportHwnd, const AppWindowState& state)
{
    if (!state.importedSceneLoaded && !state.loadedSessionActive) {
        return;
    }
    HWND parent = GetParent(viewportHwnd);
    if (!parent) {
        return;
    }

    RECT clientRect;
    GetClientRect(parent, &clientRect);
    const ViewportControlLayout controls = viewportControlLayoutForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (controls.animationValid) {
        InvalidateRect(parent, &controls.animationBarRect, FALSE);
    }
}

void refreshInteractiveViewport(HWND hwnd, AppWindowState& state)
{
    updateImportedScenePlayback(state);
    updateLoadedSessionPlayback(state);
    if (state.loadedSessionActive) {
        sampleLoadedSessionFrame(state);
    } else {
        state.poses = copyLatestPoseSnapshot(state);
    }
    renderViewport(hwnd);
    invalidateImportedAnimationControls(hwnd, state);
}

} // namespace

void handleViewportLeftButtonDown(HWND hwnd, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    const int x = GET_X_LPARAM(lparam);
    const int y = GET_Y_LPARAM(lparam);
    const ViewportPaneKind pane = paneFromClientPoint(hwnd, *state, x, y);
    state->orbitDragging = pane == ViewportPaneKind::Perspective;
    state->activeDragPane = state->orbitDragging ? pane : ViewportPaneKind::None;
    state->lastMouseX = x;
    state->lastMouseY = y;
    if (state->orbitDragging) {
        SetCapture(hwnd);
    }
}

void handleViewportLeftButtonUp(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    state->orbitDragging = false;
    if (!state->panDragging) {
        state->activeDragPane = ViewportPaneKind::None;
    }
    if (!state->panDragging) {
        ReleaseCapture();
    }
}

void handleViewportMiddleButtonDown(HWND hwnd, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    const int x = GET_X_LPARAM(lparam);
    const int y = GET_Y_LPARAM(lparam);
    const ViewportPaneKind pane = paneFromClientPoint(hwnd, *state, x, y);
    state->panDragging = pane != ViewportPaneKind::None;
    state->activeDragPane = pane;
    state->lastMouseX = x;
    state->lastMouseY = y;
    if (state->panDragging) {
        SetCapture(hwnd);
    }
}

void handleViewportMiddleButtonUp(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    state->panDragging = false;
    if (!state->orbitDragging) {
        state->activeDragPane = ViewportPaneKind::None;
    }
    if (!state->orbitDragging) {
        ReleaseCapture();
    }
}

void handleViewportMouseMove(HWND hwnd, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    const int x = GET_X_LPARAM(lparam);
    const int y = GET_Y_LPARAM(lparam);
    const int dx = x - state->lastMouseX;
    const int dy = y - state->lastMouseY;
    state->lastMouseX = x;
    state->lastMouseY = y;

    if (state->orbitDragging && state->activeDragPane == ViewportPaneKind::Perspective) {
        state->cameraYawDegrees += static_cast<float>(dx) * 0.35f;
        state->cameraPitchDegrees = clampFloat(
            state->cameraPitchDegrees + static_cast<float>(dy) * 0.25f,
            -10.0f,
            80.0f
        );
        refreshInteractiveViewport(hwnd, *state);
    } else if (state->panDragging) {
        if (state->activeDragPane == ViewportPaneKind::Perspective) {
            applyScreenSpacePan(*state, dx, dy);
        } else {
            const float unitsPerPixel = orthoWorldUnitsPerPixel(hwnd, *state, state->activeDragPane);
            applyOrthoPan(*state, state->activeDragPane, orthoPanePanOffset(state->activeDragPane, dx, dy, unitsPerPixel));
        }
        refreshInteractiveViewport(hwnd, *state);
    }
}

void handleViewportMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    ViewportPaneKind pane = ViewportPaneKind::Perspective;
    if (state->quadViewEnabled) {
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        ScreenToClient(hwnd, &point);
        pane = paneFromClientPoint(hwnd, *state, point.x, point.y);
        if (pane == ViewportPaneKind::None) {
            return;
        }
    }

    const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
    const float zoomSteps = static_cast<float>(wheelDelta) / static_cast<float>(WHEEL_DELTA);
    if (pane == ViewportPaneKind::Perspective) {
        applyCameraDolly(*state, zoomSteps * 0.45f);
    } else {
        setOrthoZoomForPane(*state, pane, orthoViewZoomAfterWheel(orthoZoomForPane(*state, pane), zoomSteps));
    }
    refreshInteractiveViewport(hwnd, *state);
}

} // namespace ovtr::win32
