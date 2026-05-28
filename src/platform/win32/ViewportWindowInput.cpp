#include "platform/win32/ViewportWindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppState.h"
#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {

void handleViewportLeftButtonDown(HWND hwnd, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    state->orbitDragging = true;
    state->lastMouseX = GET_X_LPARAM(lparam);
    state->lastMouseY = GET_Y_LPARAM(lparam);
    SetCapture(hwnd);
}

void handleViewportLeftButtonUp(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    state->orbitDragging = false;
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
    state->panDragging = true;
    state->lastMouseX = GET_X_LPARAM(lparam);
    state->lastMouseY = GET_Y_LPARAM(lparam);
    SetCapture(hwnd);
}

void handleViewportMiddleButtonUp(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }
    state->panDragging = false;
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

    if (state->orbitDragging) {
        state->cameraYawDegrees += static_cast<float>(dx) * 0.35f;
        state->cameraPitchDegrees = clampFloat(
            state->cameraPitchDegrees + static_cast<float>(dy) * 0.25f,
            -10.0f,
            80.0f
        );
        renderViewport(hwnd);
    } else if (state->panDragging) {
        applyScreenSpacePan(*state, dx, dy);
        renderViewport(hwnd);
    }
}

void handleViewportMouseWheel(HWND hwnd, WPARAM wparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
    const float zoomSteps = static_cast<float>(wheelDelta) / static_cast<float>(WHEEL_DELTA);
    applyCameraDolly(*state, zoomSteps * 0.45f);
    renderViewport(hwnd);
}

} // namespace ovtr::win32
