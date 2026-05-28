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

void resetImportedSceneRenderCache(AppViewportState& state) noexcept
{
    state.importedSceneMeshDisplayLists.clear();
}

} // namespace ovtr::win32
