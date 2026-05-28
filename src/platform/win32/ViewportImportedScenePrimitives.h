#pragma once

#include "export/RenderModelGeometry.h"

namespace ovtr::win32 {

void drawImportedGltfMeshTriangles(const ovtr::RenderModelGeometry& mesh);
void drawImportedFallbackMarker3D();

} // namespace ovtr::win32
