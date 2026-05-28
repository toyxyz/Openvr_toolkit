#pragma once

#include "import/GltfAccessor.h"
#include "import/GltfImporter.h"

#include <array>
#include <map>
#include <vector>

namespace ovtr::detail {

struct NodeAnimationValue {
    bool hasTranslation = false;
    bool hasRotation = false;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

using NodeAnimationBuilder = std::map<double, NodeAnimationValue>;

bool appendGltfAnimationChannel(
    const JsonValue& channel,
    const JsonValue& samplers,
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    std::vector<NodeAnimationBuilder>& builders,
    std::string& error
);

void applyGltfAnimationBuildersToNodes(
    std::vector<NodeAnimationBuilder>& builders,
    std::vector<ImportedGltfNode>& nodes,
    double& durationSeconds
);

} // namespace ovtr::detail
