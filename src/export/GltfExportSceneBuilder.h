#pragma once

#include "export/ExportPoseTrack.h"
#include "export/GltfExportBuffers.h"
#include "export/GltfExportModel.h"

#include <cstdint>
#include <vector>

namespace ovtr {

struct GltfExportScene {
    std::vector<GltfDevice> devices;
    std::vector<std::uint8_t> binary;
    std::vector<GltfExportBufferView> bufferViews;
    std::vector<GltfExportAccessor> accessors;
    std::vector<GltfMeshPrimitive> meshes;
    std::vector<GltfAnimationTarget> animationTargets;
};

void buildGltfExportScene(
    std::vector<ExportPoseTrack> tracks,
    double exportSampleRate,
    GltfExportScene& scene
);

} // namespace ovtr
