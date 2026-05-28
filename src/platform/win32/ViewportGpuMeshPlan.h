#pragma once

#include "export/RenderModelGeometry.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

enum class ViewportGpuIndexFormat {
    None,
    UnsignedShort,
    UnsignedInt,
};

struct ViewportGpuMeshUploadPlan {
    bool valid = false;
    ViewportGpuIndexFormat indexFormat = ViewportGpuIndexFormat::None;
    std::size_t indexCount = 0;
    std::string error;
};

ViewportGpuMeshUploadPlan planImportedGltfGpuMeshUpload(const ovtr::RenderModelGeometry& mesh);
ViewportGpuMeshUploadPlan planRenderModelGpuMeshUpload(
    std::size_t vertexCount,
    const std::vector<std::uint16_t>& indices
);

} // namespace ovtr::win32
