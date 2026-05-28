#include "platform/win32/ViewportRenderModelRenderer.h"

#include "platform/win32/AppViewportState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/ViewportCamera.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportMath.h"
#include "platform/win32/ViewportRenderModelDraw.h"
#include "platform/win32/ViewportRenderModelMatcap.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/ViewportTriangleDisplayList.h"

#include <gl/GL.h>

namespace ovtr::win32 {
namespace {

constexpr RgbColor kSelectedRenderModelOutlineColor{255, 31, 26};

void setGlColorFromRgb(const RgbColor color) noexcept
{
    const RgbColor clamped = clampRgbColor(color);
    glColor3f(
        static_cast<float>(clamped.r) / 255.0f,
        static_cast<float>(clamped.g) / 255.0f,
        static_cast<float>(clamped.b) / 255.0f
    );
}

void drawRenderModelSurface(RenderModelMesh& mesh)
{
    if (!drawCachedRenderModelTriangles(mesh.surfaceDisplayList, mesh)) {
        drawRenderModelTriangles(mesh);
    }
}

void drawRenderModelFallbackSurface(RenderModelMesh& mesh, const RgbColor materialColor)
{
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlCapability lighting(GL_LIGHTING, true);
    ScopedGlCapability light0(GL_LIGHT0, true);
    ScopedGlCapability colorMaterial(GL_COLOR_MATERIAL, true);

    const GLfloat lightPosition[4] = {0.35f, 0.85f, 0.45f, 0.0f};
    const GLfloat lightDiffuse[4] = {0.85f, 0.88f, 0.92f, 1.0f};
    const GLfloat lightAmbient[4] = {0.24f, 0.26f, 0.30f, 1.0f};
    const GLfloat materialSpecular[4] = {0.92f, 0.96f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 72.0f);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    setGlColorFromRgb(materialColor);
    drawRenderModelSurface(mesh);
}

} // namespace

bool drawSteamVRRenderModel3D(
    AppViewportState& state,
    const ovtr::PoseSample& pose,
    const ovtr::DeviceDescriptor* device,
    const int viewportHeight,
    const bool selected,
    const CameraView& cameraView,
    const float outlineWorldUnitsPerPixel
)
{
    if (device == nullptr || device->renderModelName.empty()) {
        return false;
    }

    RenderModelMesh& mesh = state.renderModelCache[device->renderModelName];
    if (!updateRenderModelMesh(mesh, device->renderModelName)) {
        return false;
    }

    ScopedGlMatrixStack modelView(GL_MODELVIEW);
    glTranslatef(pose.position[0], pose.position[1], pose.position[2]);
    multiplyOpenGLMatrixFromQuaternion(pose.rotation);

    {
        ScopedGlCapability lighting(GL_LIGHTING, false);
        ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
        ScopedGlCapability cullFace(GL_CULL_FACE, true);
        ScopedGlCullFace frontFaces(GL_FRONT);
        setGlColorFromRgb(selected ? kSelectedRenderModelOutlineColor : state.viewportSettings.renderModelOutlineColor);
        const float outlineExpansion = outlineWorldUnitsPerPixel > 0.0f
            ? outlineExpansionForOrtho(outlineWorldUnitsPerPixel, state.viewportSettings.outlineMultiplier)
            : outlineExpansionForDepth(
                cameraDepthForWorldPoint(cameraView, {pose.position[0], pose.position[1], pose.position[2]}),
                viewportHeight,
                state.viewportSettings.outlineMultiplier
            );
        drawRenderModelOutlineTriangles(mesh, outlineExpansion);
    }

    {
        ScopedGlCapability cullFace(GL_CULL_FACE, false);
        if (ensureRenderModelMatcapTexture(state)) {
            ScopedGlCapability lighting(GL_LIGHTING, false);
            ScopedGlCapability texture2D(GL_TEXTURE_2D, true);
            ScopedGlTexture2DBinding textureBinding(state.renderModelMatcapTexture.get());
            ScopedRenderModelMatcapMapping matcapMapping;
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            setGlColorFromRgb(state.viewportSettings.renderModelMaterialColor);
            drawRenderModelSurface(mesh);
        } else {
            drawRenderModelFallbackSurface(mesh, state.viewportSettings.renderModelMaterialColor);
        }
    }

    return true;
}

} // namespace ovtr::win32
