#include "platform/win32/ViewportRenderer.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ViewportCamera.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportImportedSceneRenderer.h"
#include "platform/win32/ViewportMarkerRenderer.h"
#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportOverlayRenderer.h"
#include "platform/win32/ViewportQuadView.h"
#include "platform/win32/ViewportSceneRenderer.h"
#include "platform/win32/WindowStateAccess.h"

#include <gl/GL.h>

#include <cmath>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr float kMinimumCameraDistance = 0.08f;
constexpr float kViewportFovDegrees = 48.0f;
constexpr float kPi = 3.14159265358979323846f;

struct RenderPane {
    ViewportPaneKind kind = ViewportPaneKind::Perspective;
    RECT rect{0, 0, 0, 0};
};

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

float orthoHalfHeight(const float zoom) noexcept
{
    const float distance = positiveCameraDistance(kDefaultCameraDistance, kMinimumCameraDistance);
    return distance * std::tan(kViewportFovDegrees * kPi / 360.0f) / clampOrthoViewZoom(zoom);
}

CameraView cameraForPane(const AppViewportState& state, const ViewportPaneKind pane) noexcept
{
    switch (pane) {
    case ViewportPaneKind::Front:
        return {0.0f, 0.0f, kDefaultCameraDistance, {
            state.frontViewPanX,
            state.frontViewPanY,
            0.0f
        }};
    case ViewportPaneKind::Top:
        return {0.0f, 90.0f, kDefaultCameraDistance, {
            state.topViewPanX,
            0.0f,
            state.topViewPanZ
        }};
    case ViewportPaneKind::Left:
        return {90.0f, 0.0f, kDefaultCameraDistance, {
            0.0f,
            state.leftViewPanY,
            state.leftViewPanZ
        }};
    case ViewportPaneKind::Perspective:
    case ViewportPaneKind::None:
    default:
        return cameraViewFromState(state);
    }
}

void applyPerspectiveProjection(const int width, const int height)
{
    float projection[16];
    perspectiveMatrix(kViewportFovDegrees, static_cast<float>(width) / static_cast<float>(height), 0.05f, 100.0f, projection);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
}

float applyOrthoProjection(const int width, const int height, const float halfHeight)
{
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(
        -halfHeight * aspect,
        halfHeight * aspect,
        -halfHeight,
        halfHeight,
        -100.0,
        100.0
    );
    return (2.0f * halfHeight) / static_cast<float>(height);
}

void applyCameraTransform(const CameraView& camera, const bool perspective)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (perspective) {
        glTranslatef(0.0f, -0.55f, -positiveCameraDistance(camera.distance, kMinimumCameraDistance));
    }
    glRotatef(camera.pitchDegrees, 1.0f, 0.0f, 0.0f);
    glRotatef(camera.yawDegrees, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera.pan.x, -camera.pan.y, -camera.pan.z);
}

void drawScene3D(AppWindowState& state, const int paneHeight, const CameraView& camera, const float orthoPixelSize)
{
    ScopedGlCapability depthTest(GL_DEPTH_TEST, true);
    drawGroundGrid3D(
        state.viewportSettings.gridColor,
        state.viewportSettings.gridSize,
        state.viewportSettings.gridCellDensity
    );
    drawAxes3D();
    drawTrackedDevices3D(
        static_cast<AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state),
        static_cast<const AppOriginState&>(state),
        static_cast<AppViewportState&>(state),
        paneHeight,
        camera,
        orthoPixelSize
    );
    drawImportedGltfScene3D(state);
    drawSceneMarkers3D(
        static_cast<const AppMarkerState&>(state),
        static_cast<AppViewportState&>(state),
        paneHeight,
        camera,
        orthoPixelSize
    );
}

void drawLabels3D(AppWindowState& state)
{
    ScopedGlCapability depthTest(GL_DEPTH_TEST, false);
    if (!state.deviceLabelsVisible) {
        return;
    }
    setGlColor(state.viewportSettings.labelTextColor);
    drawTrackedDeviceLabels3D(state);
    drawSceneMarkerLabels3D(
        static_cast<const AppMarkerState&>(state),
        static_cast<const AppViewportState&>(state)
    );
}

void renderPane(AppWindowState& state, const int fullHeight, const RenderPane& pane)
{
    const int paneWidth = pane.rect.right - pane.rect.left;
    const int paneHeight = pane.rect.bottom - pane.rect.top;
    if (paneWidth <= 0 || paneHeight <= 0) {
        return;
    }

    const int glY = fullHeight - pane.rect.bottom;
    glViewport(pane.rect.left, glY, paneWidth, paneHeight);
    glScissor(pane.rect.left, glY, paneWidth, paneHeight);
    setGlClearColor(state.viewportSettings.backgroundColor);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const bool perspective = pane.kind == ViewportPaneKind::Perspective;
    const CameraView camera = cameraForPane(state, pane.kind);
    float orthoPixelSize = 0.0f;
    if (perspective) {
        applyPerspectiveProjection(paneWidth, paneHeight);
    } else {
        orthoPixelSize = applyOrthoProjection(paneWidth, paneHeight, orthoHalfHeight(orthoZoomForPane(state, pane.kind)));
    }
    applyCameraTransform(camera, perspective);
    drawScene3D(state, paneHeight, camera, orthoPixelSize);
    drawLabels3D(state);
}

void drawViewportText2D(const std::string& text, const GLuint fontBase, const float x, const float y)
{
    if (text.empty() || fontBase == 0) {
        return;
    }
    glRasterPos2f(x, y);
    drawLabelText3D(text, fontBase);
}

void drawQuadViewDecorations(const AppViewportState& state, const QuadViewLayout& layout, const int width, const int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ScopedGlCapability depthTest(GL_DEPTH_TEST, false);
    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlLineWidth lineWidth(1.0f);
    setGlColor(state.viewportSettings.gridColor);
    glBegin(GL_LINES);
    glVertex2f(static_cast<float>(layout.frontRect.left), 0.0f);
    glVertex2f(static_cast<float>(layout.frontRect.left), static_cast<float>(height));
    glVertex2f(0.0f, static_cast<float>(layout.topRect.top));
    glVertex2f(static_cast<float>(width), static_cast<float>(layout.topRect.top));
    glEnd();

    setGlColor(state.viewportSettings.labelTextColor);
    drawViewportText2D(quadViewPaneLabel(ViewportPaneKind::Perspective), state.glLabelFontBase.get(), 10.0f, 22.0f);
    drawViewportText2D(quadViewPaneLabel(ViewportPaneKind::Front), state.glLabelFontBase.get(), static_cast<float>(layout.frontRect.left + 10), 22.0f);
    drawViewportText2D(quadViewPaneLabel(ViewportPaneKind::Top), state.glLabelFontBase.get(), 10.0f, static_cast<float>(layout.topRect.top + 22));
    drawViewportText2D(quadViewPaneLabel(ViewportPaneKind::Left), state.glLabelFontBase.get(), static_cast<float>(layout.leftRect.left + 10), static_cast<float>(layout.leftRect.top + 22));
}

void renderSingleViewport(AppWindowState& state, const int width, const int height)
{
    const RenderPane pane{ViewportPaneKind::Perspective, RECT{0, 0, width, height}};
    ScopedGlCapability scissor(GL_SCISSOR_TEST, true);
    renderPane(state, height, pane);
}

void renderQuadViewport(AppWindowState& state, const int width, const int height)
{
    const QuadViewLayout layout = quadViewLayoutForViewport(width, height);
    if (!layout.valid) {
        renderSingleViewport(state, width, height);
        return;
    }

    const RenderPane panes[] = {
        {ViewportPaneKind::Perspective, layout.perspectiveRect},
        {ViewportPaneKind::Front, layout.frontRect},
        {ViewportPaneKind::Top, layout.topRect},
        {ViewportPaneKind::Left, layout.leftRect},
    };
    ScopedGlCapability scissor(GL_SCISSOR_TEST, true);
    for (const RenderPane& pane : panes) {
        renderPane(state, height, pane);
    }
    glDisable(GL_SCISSOR_TEST);
    drawQuadViewDecorations(state, layout, width, height);
}

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
    if (state->quadViewEnabled) {
        renderQuadViewport(*state, width, height);
    } else {
        renderSingleViewport(*state, width, height);
    }

    glViewport(0, 0, width, height);
    drawViewportOverlays2D(*state, width, height);
    SwapBuffers(state->glDeviceContext.get());
    ++state->renderFrames;
}

} // namespace ovtr::win32
