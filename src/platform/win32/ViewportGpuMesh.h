#pragma once

#include "platform/win32/Win32GlBufferResource.h"

#include <glad/glad.h>

#include <string>

namespace ovtr::win32 {

struct ViewportGpuMesh {
    UniqueGlBuffer vertexBuffer;
    UniqueGlBuffer indexBuffer;
    GLenum indexType = 0;
    GLsizei indexCount = 0;
    bool uploadFailed = false;
    std::string failureReason;
};

void resetViewportGpuMesh(ViewportGpuMesh& mesh) noexcept;

} // namespace ovtr::win32
