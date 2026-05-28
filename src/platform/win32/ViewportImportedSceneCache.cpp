#include "platform/win32/ViewportImportedSceneCache.h"

namespace ovtr::win32 {

ViewportTriangleDisplayListCache& importedSceneMeshCache(
    AppViewportState& state,
    const std::size_t meshIndex
)
{
    if (state.importedSceneMeshDisplayLists.size() <= meshIndex) {
        state.importedSceneMeshDisplayLists.resize(meshIndex + 1u);
    }
    return state.importedSceneMeshDisplayLists[meshIndex];
}

ViewportGpuMesh& importedSceneGpuMeshCache(
    AppViewportState& state,
    const std::size_t meshIndex
)
{
    if (state.importedSceneGpuMeshes.size() <= meshIndex) {
        state.importedSceneGpuMeshes.resize(meshIndex + 1u);
    }
    return state.importedSceneGpuMeshes[meshIndex];
}

void resetImportedSceneRenderCache(AppViewportState& state) noexcept
{
    state.importedSceneGpuMeshes.clear();
    state.importedSceneMeshDisplayLists.clear();
}

} // namespace ovtr::win32
