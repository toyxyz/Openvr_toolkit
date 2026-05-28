#pragma once

#include "platform/win32/AppViewportState.h"
#include "platform/win32/ViewportTriangleDisplayListCache.h"

#include <cstddef>

namespace ovtr::win32 {

ViewportTriangleDisplayListCache& importedSceneMeshCache(
    AppViewportState& state,
    std::size_t meshIndex
);

void resetImportedSceneRenderCache(AppViewportState& state) noexcept;

} // namespace ovtr::win32
