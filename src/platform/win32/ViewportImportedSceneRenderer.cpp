#include "platform/win32/ViewportImportedSceneRenderer.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppViewportState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/ViewportDrawPrimitives.h"
#include "platform/win32/ViewportGlMatrixScope.h"
#include "platform/win32/ViewportGlStateScope.h"
#include "platform/win32/ViewportImportedScenePrimitives.h"

#include <gl/GL.h>

#include <array>

namespace ovtr::win32 {

void drawImportedGltfScene3D(
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState
)
{
    if (!importedSceneState.importedSceneLoaded) {
        return;
    }

    const double playbackTime = importedScenePlaybackTime(importedSceneState);

    ScopedGlCapability lighting(GL_LIGHTING, false);
    ScopedGlCapability texture2D(GL_TEXTURE_2D, false);
    ScopedGlCapability cullFace(GL_CULL_FACE, false);
    ScopedGlDepthFunc depthFunc(GL_LEQUAL);
    setGlColor(viewportState.viewportSettings.importedGlbColor);

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
            const ovtr::RenderModelGeometry& mesh =
                importedSceneState.importedScene.meshes[static_cast<std::size_t>(node.meshIndex)];
            if (mesh.available) {
                drawImportedGltfMeshTriangles(mesh);
            } else {
                drawImportedFallbackMarker3D();
            }
        } else {
            drawImportedFallbackMarker3D();
        }
    }
}

} // namespace ovtr::win32
