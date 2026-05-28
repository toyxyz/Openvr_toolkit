#pragma once

#include "export/RenderModelGeometry.h"
#include "platform/win32/ViewportGpuMesh.h"
#include "platform/win32/ViewportRenderModelTypes.h"

namespace ovtr::win32 {

bool uploadImportedGltfGpuMesh(ViewportGpuMesh& gpuMesh, const ovtr::RenderModelGeometry& mesh);
bool uploadRenderModelGpuMesh(ViewportGpuMesh& gpuMesh, const RenderModelMesh& mesh);

} // namespace ovtr::win32
