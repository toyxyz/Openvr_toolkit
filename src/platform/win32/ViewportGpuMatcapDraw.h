#pragma once

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/ViewportGpuMesh.h"

namespace ovtr::win32 {

struct AppViewportState;

bool drawGpuMatcapMesh(
    AppViewportState& state,
    const ViewportGpuMesh& mesh,
    GLuint matcapTexture,
    RgbColor tintColor
);

} // namespace ovtr::win32
