#include "platform/win32/WindowKeyboardSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/FrameUpdate.h"
#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportRenderer.h"

namespace ovtr::win32 {

bool handleCameraKeyDown(HWND hwnd, AppWindowState* state, const WPARAM wparam)
{
    if (wparam == VK_LEFT) {
        if (state) {
            state->cameraYawDegrees -= 5.0f;
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    if (wparam == VK_RIGHT) {
        if (state) {
            state->cameraYawDegrees += 5.0f;
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    if (wparam == VK_UP) {
        if (state) {
            state->cameraPitchDegrees = clampFloat(state->cameraPitchDegrees - 4.0f, -10.0f, 80.0f);
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    if (wparam == VK_DOWN) {
        if (state) {
            state->cameraPitchDegrees = clampFloat(state->cameraPitchDegrees + 4.0f, -10.0f, 80.0f);
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    if (wparam == VK_OEM_PLUS || wparam == VK_ADD) {
        if (state) {
            applyCameraDolly(*state, 0.35f);
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    if (wparam == VK_OEM_MINUS || wparam == VK_SUBTRACT) {
        if (state) {
            applyCameraDolly(*state, -0.35f);
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    if (wparam == VK_F3) {
        if (state) {
            state->cameraYawDegrees = kDefaultCameraYawDegrees;
            state->cameraPitchDegrees = kDefaultCameraPitchDegrees;
            state->cameraDistance = kDefaultCameraDistance;
            state->cameraPanX = kDefaultCameraPanX;
            state->cameraPanY = kDefaultCameraPanY;
            state->cameraPanZ = kDefaultCameraPanZ;
            state->frontViewPanX = 0.0f;
            state->frontViewPanY = 0.0f;
            state->frontViewZoom = kDefaultOrthoViewZoom;
            state->topViewPanX = 0.0f;
            state->topViewPanZ = 0.0f;
            state->topViewZoom = kDefaultOrthoViewZoom;
            state->leftViewPanZ = 0.0f;
            state->leftViewPanY = 0.0f;
            state->leftViewZoom = kDefaultOrthoViewZoom;
            state->orbitDragging = false;
            state->panDragging = false;
            state->activeDragPane = ViewportPaneKind::None;
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    return false;
}

} // namespace ovtr::win32
