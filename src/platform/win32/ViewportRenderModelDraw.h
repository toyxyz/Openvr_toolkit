#pragma once

#include "platform/win32/ViewportRenderModelTypes.h"

namespace ovtr::win32 {

void drawRenderModelTriangles(const RenderModelMesh& mesh);
void drawRenderModelOutlineTriangles(const RenderModelMesh& mesh, float expansion);

} // namespace ovtr::win32
