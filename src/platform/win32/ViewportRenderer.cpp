#include "platform/win32/ViewportRenderer.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportImportedSceneRenderer.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportOverlayRenderer.h"
#include "platform/win32/ViewportSceneRenderer.h"
#include "platform/win32/WindowStateAccess.h"

#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr float kMinimumCameraDistance = 0.08f;
constexpr float kViewportFovDegrees = 48.0f;

} // namespace

void renderViewport(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state || !state->glDeviceContext || !state->glContext) {
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }

    wglMakeCurrent(state->glDeviceContext.get(), state->glContext.get());
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float projection[16];
    perspectiveMatrix(
        kViewportFovDegrees,
        static_cast<float>(width) / static_cast<float>(height),
        0.05f,
        100.0f,
        projection
    );
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    setGlClearColor(state->viewportSettings.backgroundColor);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTranslatef(
        0.0f,
        -0.55f,
        -positiveCameraDistance(state->cameraDistance, kMinimumCameraDistance)
    );
    glRotatef(state->cameraPitchDegrees, 1.0f, 0.0f, 0.0f);
    glRotatef(state->cameraYawDegrees, 0.0f, 1.0f, 0.0f);
    glTranslatef(-state->cameraPanX, -state->cameraPanY, -state->cameraPanZ);

    {
        ScopedGlCapability depthTest(GL_DEPTH_TEST, true);
        drawGroundGrid3D(state->viewportSettings.gridColor);
        drawAxes3D();

        drawTrackedDevices3D(*state, height);
        drawImportedGltfScene3D(*state);
    }

    {
        ScopedGlCapability depthTest(GL_DEPTH_TEST, false);
        setGlColor(state->viewportSettings.labelTextColor);
        drawTrackedDeviceLabels3D(*state);

        drawViewportOverlays2D(*state, width, height);
    }

    SwapBuffers(state->glDeviceContext.get());
    ++state->renderFrames;
}

} // namespace ovtr::win32
