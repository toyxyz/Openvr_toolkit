#pragma once

#include "data/SessionTypes.h"
#include "export/GltfExportBuffers.h"
#include "export/GltfExportModel.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ovtr {

std::string makeGltfJson(
    const RecordingSession& session,
    const std::vector<GltfDevice>& devices,
    const std::vector<GltfMeshPrimitive>& meshes,
    const std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<GltfExportAccessor>& accessors,
    const std::vector<GltfAnimationTarget>& animationTargets,
    std::size_t binaryByteLength,
    const std::string& bufferUri
);

} // namespace ovtr
