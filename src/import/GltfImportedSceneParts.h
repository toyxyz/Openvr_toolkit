#pragma once

#include "import/GltfAccessor.h"
#include "import/GltfImporter.h"
#include "import/GltfJson.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

bool parseImportedGltfMeshes(
    const JsonValue& root,
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    ImportedGltfScene& scene,
    std::string& error
);

std::vector<ImportedGltfNode> parseImportedGltfNodes(const JsonValue& root);

bool parseImportedGltfAnimations(
    const JsonValue& root,
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    std::vector<ImportedGltfNode>& nodes,
    double& durationSeconds,
    std::string& error
);

} // namespace ovtr
