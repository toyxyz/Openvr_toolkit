#include "platform/win32/ViewportGpuMeshPlan.h"

#include <cstdint>
#include <vector>

namespace ovtr::win32 {

ViewportGpuMeshUploadPlan planImportedGltfGpuMeshUpload(const ovtr::RenderModelGeometry& mesh)
{
    if (!mesh.available || mesh.vertices.empty() || mesh.indices.empty()) {
        return {false, ViewportGpuIndexFormat::None, 0, "mesh has no available indexed triangles"};
    }

    for (const std::uint32_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            return {false, ViewportGpuIndexFormat::None, 0, "mesh index exceeds vertex count"};
        }
    }

    return {true, ViewportGpuIndexFormat::UnsignedInt, mesh.indices.size(), {}};
}

ViewportGpuMeshUploadPlan planRenderModelGpuMeshUpload(
    const std::size_t vertexCount,
    const std::vector<std::uint16_t>& indices
)
{
    if (vertexCount == 0 || indices.empty()) {
        return {false, ViewportGpuIndexFormat::None, 0, "render model has no indexed triangles"};
    }

    for (const std::uint16_t index : indices) {
        if (index >= vertexCount) {
            return {false, ViewportGpuIndexFormat::None, 0, "render model index exceeds vertex count"};
        }
    }

    return {true, ViewportGpuIndexFormat::UnsignedShort, indices.size(), {}};
}

} // namespace ovtr::win32
