#include "import/GltfImportedSceneParts.h"

#include "import/GltfJsonUtils.h"
#include "math/QuaternionUtils.h"

#include <utility>

namespace ovtr {

std::vector<ImportedGltfNode> parseImportedGltfNodes(const JsonValue& root)
{
    std::vector<ImportedGltfNode> nodes;
    const JsonValue* nodesValue = root.find("nodes");
    if (!nodesValue || nodesValue->type != JsonValue::Type::Array) {
        return nodes;
    }

    nodes.reserve(nodesValue->arrayValue.size());
    for (std::size_t nodeIndex = 0; nodeIndex < nodesValue->arrayValue.size(); ++nodeIndex) {
        ImportedGltfNode node;
        node.nodeIndex = static_cast<int>(nodeIndex);
        const JsonValue& nodeValue = nodesValue->arrayValue[nodeIndex];
        if (nodeValue.type == JsonValue::Type::Object) {
            node.name = jsonStringMember(nodeValue, "name");
            node.meshIndex = jsonIntMember(nodeValue, "mesh", -1);
            readJsonFloatArray(nodeValue, "translation", node.translation.data(), node.translation.size());
            readJsonFloatArray(nodeValue, "rotation", node.rotation.data(), node.rotation.size());
            readJsonFloatArray(nodeValue, "scale", node.scale.data(), node.scale.size());
            node.rotation = normalizeQuaternion(node.rotation);
        }
        nodes.push_back(std::move(node));
    }
    return nodes;
}

} // namespace ovtr
