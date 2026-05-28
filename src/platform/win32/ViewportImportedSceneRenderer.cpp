#include "platform/win32/ViewportImportedSceneRenderer.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGpuMatcapDraw.h"
#include "platform/win32/ViewportGpuMeshUpload.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportImportedSceneCache.h"
#include "platform/win32/ViewportGlTextureBindingScope.h"
#include "platform/win32/ViewportImportedScenePrimitives.h"
#include "platform/win32/ViewportRenderModelMatcap.h"
#include "platform/win32/ViewportTriangleDisplayList.h"

#include <gl/GL.h>

#include <array>
#include <cstddef>

namespace ovtr::win32 {
namespace {

void drawImportedSceneNodes(
    const AppImportedSceneState& importedSceneState,
    AppViewportState& viewportState,
    const double playbackTime,
    const GLuint matcapTexture
)
{
    for (const ovtr::ImportedGltfNode& node : importedSceneState.importedScene.nodes) {
        std::array<float, 3> translation{};
        std::array<float, 4> rotation{};
        ovtr::sampleImportedGltfNodePose(node, playbackTime, translation, rotation);

        ScopedGlMatrixStack modelView(GL_MODELVIEW);
        glTranslatef(translation[0], translation[1], translation[2]);
        multiplyOpenGLMatrixFromQuaternion(rotation);
        glScalef(node.scale[0], node.scale[1], node.scale[2]);
        setGlColor(viewportState.viewportSettings.importedGlbColor);

        if (node.meshIndex >= 0 &&
            static_cast<std::size_t>(node.meshIndex) < importedSceneState.importedScene.meshes.size()) {
            const std::size_t meshIndex = static_cast<std::size_t>(node.meshIndex);
            const ovtr::RenderModelGeometry& mesh = importedSceneState.importedScene.meshes[meshIndex];
            if (mesh.available) {
                ViewportGpuMesh& gpuMesh = importedSceneGpuMeshCache(viewportState, meshIndex);
                if (matcapTexture != 0 &&
                    uploadImportedGltfGpuMesh(gpuMesh, mesh) &&
                    drawGpuMatcapMesh(viewportState, gpuMesh, matcapTexture, viewportState.viewportSettings.importedGlbColor)) {
                    continue;
                }
                ViewportTriangleDisplayListCache& cache = importedSceneMeshCache(viewportState, meshIndex);
                if (matcapTexture != 0) {
                    ScopedGlCapability texture2D(GL_TEXTURE_2D, true);
                    ScopedGlTexture2DBinding textureBinding(matcapTexture);
                    ScopedRenderModelMatcapMapping matcapMapping;
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    if (!drawCachedImportedGltfMeshTriangles(cache, mesh)) {
                        drawImportedGltfMeshTriangles(mesh);
                    }
                    continue;
                }
                if (!drawCachedImportedGltfMeshTriangles(cache, mesh)) {
                    drawImportedGltfMeshTriangles(mesh);
                }
            } else {
                drawImportedFallbackMarker3D();
            }
        } else {
            drawImportedFallbackMarker3D();
        }
    }
}

} // namespace

void drawImportedGltfScene3D(
    const AppImportedSceneState& importedSceneState,
    AppViewportState& viewportState
)
{
    if (!importedSceneState.importedSceneLoaded) {
        return;
    }

    const double playbackTime = importedScenePlaybackTime(importedSceneState);
    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    ScopedGlDepthFunc depthFunc(GL_LEQUAL);
    ScopedGlCapability lighting(GL_LIGHTING, false);

    const GLuint matcapTexture = ensureRenderModelMatcapTexture(viewportState)
        ? viewportState.renderModelMatcapTexture.get()
        : 0;
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    drawImportedSceneNodes(importedSceneState, viewportState, playbackTime, matcapTexture);
}

} // namespace ovtr::win32
