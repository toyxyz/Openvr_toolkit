#pragma once

#include "data/SessionTypes.h"
#include "export/GltfExportBuffers.h"
#include "export/GltfExportModel.h"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr::detail {

void writeGltfJsonNodes(std::ostream& out, const std::vector<GltfDevice>& devices);
void writeGltfJsonMeshesAndMaterials(std::ostream& out, const std::vector<GltfMeshPrimitive>& meshes);
void writeGltfJsonAnimation(
    std::ostream& out,
    const RecordingSession& session,
    const std::vector<GltfAnimationTarget>& animationTargets
);
void writeGltfJsonBufferMetadata(
    std::ostream& out,
    const std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<GltfExportAccessor>& accessors,
    std::size_t binaryByteLength,
    const std::string& bufferUri
);
void writeGltfJsonExtras(std::ostream& out, const RecordingSession& session);

} // namespace ovtr::detail
