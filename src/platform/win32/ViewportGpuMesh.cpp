#include "platform/win32/ViewportGpuMesh.h"

namespace ovtr::win32 {

void resetViewportGpuMesh(ViewportGpuMesh& mesh) noexcept
{
    mesh.vertexBuffer.reset();
    mesh.indexBuffer.reset();
    mesh.indexType = 0;
    mesh.indexCount = 0;
    mesh.uploadFailed = false;
    mesh.failureReason.clear();
}

} // namespace ovtr::win32
