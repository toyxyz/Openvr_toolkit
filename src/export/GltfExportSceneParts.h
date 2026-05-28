#pragma once

#include "export/GltfExportBuffers.h"
#include "export/GltfExportModel.h"

#include <cstdint>
#include <vector>

namespace ovtr {

void resampleGltfKeys(std::vector<GltfKey>& keys, double sampleRate);

void buildGltfMeshData(
    std::vector<GltfDevice>& devices,
    std::vector<std::uint8_t>& binary,
    std::vector<GltfExportBufferView>& bufferViews,
    std::vector<GltfExportAccessor>& accessors,
    std::vector<GltfMeshPrimitive>& meshes
);

void buildGltfAnimationData(
    std::vector<GltfDevice>& devices,
    std::vector<std::uint8_t>& binary,
    std::vector<GltfExportBufferView>& bufferViews,
    std::vector<GltfExportAccessor>& accessors,
    std::vector<GltfAnimationTarget>& animationTargets
);

} // namespace ovtr
