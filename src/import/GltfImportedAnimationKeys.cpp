#include "import/GltfImportedAnimationBuilders.h"

#include "math/QuaternionUtils.h"

#include <algorithm>

namespace ovtr::detail {

void applyGltfAnimationBuildersToNodes(
    std::vector<NodeAnimationBuilder>& builders,
    std::vector<ImportedGltfNode>& nodes,
    double& durationSeconds
)
{
    for (std::size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
        ImportedGltfNode& node = nodes[nodeIndex];
        NodeAnimationBuilder& builder = builders[nodeIndex];
        if (builder.empty()) {
            continue;
        }

        std::array<float, 3> translation = node.translation;
        std::array<float, 4> rotation = node.rotation;
        node.keys.clear();
        node.keys.reserve(builder.size());
        for (const auto& entry : builder) {
            if (entry.second.hasTranslation) {
                translation = entry.second.translation;
            }
            if (entry.second.hasRotation) {
                rotation = entry.second.rotation;
            }

            node.keys.push_back(ImportedGltfKey{
                entry.first,
                translation,
                normalizeQuaternion(rotation),
            });
            durationSeconds = std::max(durationSeconds, entry.first);
        }
    }
}

} // namespace ovtr::detail
