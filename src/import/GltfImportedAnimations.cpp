#include "import/GltfImportedSceneParts.h"

#include "import/GltfImportedAnimationBuilders.h"
#include "import/GltfJsonUtils.h"

namespace ovtr {

bool parseImportedGltfAnimations(
    const JsonValue& root,
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    std::vector<ImportedGltfNode>& nodes,
    double& durationSeconds,
    std::string& error
)
{
    durationSeconds = 0.0;
    if (nodes.empty()) {
        return true;
    }

    std::vector<detail::NodeAnimationBuilder> builders(nodes.size());
    const JsonValue* animations = root.find("animations");
    if (!animations || animations->type != JsonValue::Type::Array) {
        return true;
    }

    for (const JsonValue& animation : animations->arrayValue) {
        if (animation.type != JsonValue::Type::Object) {
            error = "glTF animation must be an object";
            return false;
        }
        const JsonValue* samplers = animation.find("samplers");
        const JsonValue* channels = animation.find("channels");
        if (!samplers || !channels ||
            samplers->type != JsonValue::Type::Array ||
            channels->type != JsonValue::Type::Array) {
            continue;
        }

        for (const JsonValue& channel : channels->arrayValue) {
            if (!detail::appendGltfAnimationChannel(
                    channel,
                    *samplers,
                    bufferViews,
                    accessors,
                    binary,
                    builders,
                    error
                )) {
                return false;
            }
        }
    }

    detail::applyGltfAnimationBuildersToNodes(builders, nodes, durationSeconds);
    return true;
}

} // namespace ovtr
