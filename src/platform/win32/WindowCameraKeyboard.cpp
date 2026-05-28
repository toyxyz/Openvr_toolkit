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
    if (wparam == VK_HOME) {
        if (state) {
            state->cameraYawDegrees = 42.0f;
            state->cameraPitchDegrees = 28.0f;
            state->cameraDistance = 5.5f;
            state->cameraPanX = 0.0f;
            state->cameraPanY = 0.0f;
            state->cameraPanZ = 0.0f;
            refreshPoseAndViewport(hwnd);
        }
        return true;
    }
    return false;
}

} // namespace ovtr::win32
