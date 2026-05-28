#include "platform/win32/ViewportRenderModelCleanup.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ViewportImportedSceneCache.h"
#include "platform/win32/ViewportRenderModelMatcap.h"
#include "platform/win32/ViewportTriangleDisplayListCache.h"

namespace ovtr::win32 {

void deleteRenderModelTextures(AppWindowState& state) noexcept
{
    for (auto& entry : state.renderModelCache) {
        RenderModelMesh& mesh = entry.second;
        resetTriangleDisplayListCache(mesh.surfaceDisplayList);
        if (mesh.texture) {
            mesh.texture.reset();
            mesh.textureAvailable = false;
        }
    }
    resetImportedSceneRenderCache(state);
    deleteRenderModelMatcapTexture(state);
}

} // namespace ovtr::win32
