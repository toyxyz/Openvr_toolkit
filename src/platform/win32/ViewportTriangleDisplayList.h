#pragma once

#include "export/RenderModelGeometry.h"
#include "platform/win32/ViewportRenderModelTypes.h"
#include "platform/win32/ViewportTriangleDisplayListCache.h"

namespace ovtr::win32 {

bool drawCachedImportedGltfMeshTriangles(
    ViewportTriangleDisplayListCache& cache,
    const ovtr::RenderModelGeometry& mesh
);

bool drawCachedRenderModelTriangles(
    ViewportTriangleDisplayListCache& cache,
    const RenderModelMesh& mesh
);

} // namespace ovtr::win32
