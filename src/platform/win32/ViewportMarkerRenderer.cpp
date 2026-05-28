#include "platform/win32/ViewportMarkerRenderer.h"

#include "platform/win32/ViewportCamera.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportRenderModelMatcap.h"

#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr RgbColor kSelectedMarkerOutlineColor{255, 31, 26};

void drawCubeFaces(const float halfSize)
{
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);

    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(halfSize, -halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, -halfSize);
    glVertex3f(halfSize, halfSize, halfSize);
    glVertex3f(halfSize, -halfSize, halfSize);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-halfSize, -halfSize, -halfSize);
    glVertex3f(-halfSize, -halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, halfSize);
    glVertex3f(-halfSize, halfSize, -halfSize);
    glEnd();
}

float markerOutlineExpansion(
    const SceneMarker& marker,
    const AppViewportState& state,
    const int viewportHeight,
    const CameraView& cameraView,
    const float outlineWorldUnitsPerPixel
)
{
    if (outlineWorldUnitsPerPixel > 0.0f) {
        return outlineExpansionForOrtho(outlineWorldUnitsPerPixel, state.viewportSettings.outlineMultiplier);
    }
    return outlineExpansionForDepth(
        cameraDepthForWorldPoint(cameraView, {marker.position[0], marker.position[1], marker.position[2]}),
        viewportHeight,
        state.viewportSettings.outlineMultiplier
    );
}

void drawMarker3D(
    const SceneMarker& marker,
    AppViewportState& state,
    const bool selected,
    const int viewportHeight,
    const CameraView& cameraView,
    const float outlineWorldUnitsPerPixel
)
{
    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glTranslatef(marker.position[0], marker.position[1], marker.position[2]);
    multiplyOpenGLMatrixFromQuaternion(marker.rotation);

    const float halfSize = marker.sizeMeters * 0.5f;
    {
        ScopedGlCapability lighting(GL_LIGHTING, false);
        ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
        ScopedGlCapability cullFace(GL_CULL_FACE, true);
        ScopedGlCullFace frontFaces(GL_FRONT);
        setGlColor(selected ? kSelectedMarkerOutlineColor : state.viewportSettings.renderModelOutlineColor);
        drawCubeFaces(halfSize + markerOutlineExpansion(marker, state, viewportHeight, cameraView, outlineWorldUnitsPerPixel));
    }

    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    if (ensureRenderModelMatcapTexture(state)) {
        ScopedGlCapability lighting(GL_LIGHTING, false);
        ScopedGlCapability texture2D(GL_TEXTURE_2D, true);
        ScopedGlTexture2DBinding textureBinding(state.renderModelMatcapTexture.get());
        ScopedRenderModelMatcapMapping matcapMapping;
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        setGlColor(state.viewportSettings.renderModelMaterialColor);
        drawCubeFaces(halfSize);
    } else {
        ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
        setGlColor(state.viewportSettings.renderModelMaterialColor);
        drawCubeFaces(halfSize);
    }
}

} // namespace

void drawSceneMarkers3D(
    const AppMarkerState& markerState,
    AppViewportState& viewportState,
    const int viewportHeight,
    const CameraView& cameraView,
    const float outlineWorldUnitsPerPixel
)
{
    for (const SceneMarker& marker : markerState.markers) {
        drawMarker3D(
            marker,
            viewportState,
            marker.id == markerState.selectedMarkerId,
            viewportHeight,
            cameraView,
            outlineWorldUnitsPerPixel
        );
    }
}

} // namespace ovtr::win32
