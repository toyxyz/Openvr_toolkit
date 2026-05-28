#include "import/GltfImportedSceneBuilder.h"

#include "import/GltfAccessor.h"
#include "import/GltfImportedSceneParts.h"

#include <utility>
#include <vector>

namespace ovtr {

bool buildImportedGltfScene(
    const JsonValue& root,
    const GltfGlbPayload& payload,
    const std::filesystem::path& inputPath,
    ImportedGltfScene& scene,
    std::string& error
)
{
    std::vector<GltfBufferView> bufferViews;
    if (!parseGltfBufferViews(root, bufferViews, error)) {
        return false;
    }
    std::vector<GltfAccessor> accessors;
    if (!parseGltfAccessors(root, accessors, error)) {
        return false;
    }

    ImportedGltfScene builtScene;
    builtScene.sourcePath = inputPath;
    builtScene.name = inputPath.stem().string();
    if (!parseImportedGltfMeshes(root, bufferViews, accessors, payload.binary, builtScene, error)) {
        return false;
    }

    std::vector<ImportedGltfNode> allNodes = parseImportedGltfNodes(root);
    if (!parseImportedGltfAnimations(
            root,
            bufferViews,
            accessors,
            payload.binary,
            allNodes,
            builtScene.durationSeconds,
            error
        )) {
        return false;
    }

    for (ImportedGltfNode& node : allNodes) {
        if (node.meshIndex >= 0 || !node.keys.empty()) {
            builtScene.nodes.push_back(std::move(node));
        }
    }

    scene = std::move(builtScene);
    return true;
}

} // namespace ovtr
