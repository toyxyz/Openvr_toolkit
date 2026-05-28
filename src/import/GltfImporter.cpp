#include "import/GltfImporter.h"

#include "import/GltfGlbReader.h"
#include "import/GltfImportedSceneBuilder.h"
#include "import/GltfJson.h"
#include "math/PoseInterpolation.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <utility>

namespace ovtr {

GltfImportResult importGlbScene(const std::filesystem::path& inputPath)
{
    GltfImportResult result;
    result.inputPath = inputPath;

    GltfGlbPayload payload;
    if (!readGltfGlbPayload(inputPath, payload, result.error)) {
        return result;
    }

    JsonValue root;
    JsonParser parser(payload.json);
    if (!parser.parse(root)) {
        result.error = "failed to parse GLB JSON: " + parser.error();
        return result;
    }
    if (root.type != JsonValue::Type::Object) {
        result.error = "GLB JSON root must be an object";
        return result;
    }

    ImportedGltfScene scene;
    if (!buildImportedGltfScene(root, payload, inputPath, scene, result.error)) {
        return result;
    }
    if (scene.nodes.empty()) {
        result.error = "GLB contains no visible or animated nodes";
        return result;
    }

    result.scene = std::move(scene);
    result.success = true;
    return result;
}

bool sampleImportedGltfNodePose(
    const ImportedGltfNode& node,
    const double timeSeconds,
    std::array<float, 3>& translation,
    std::array<float, 4>& rotation
)
{
    if (node.keys.empty()) {
        translation = node.translation;
        rotation = node.rotation;
        return true;
    }

    if (timeSeconds <= node.keys.front().timeSeconds) {
        translation = node.keys.front().translation;
        rotation = node.keys.front().rotation;
        return true;
    }
    if (timeSeconds >= node.keys.back().timeSeconds) {
        translation = node.keys.back().translation;
        rotation = node.keys.back().rotation;
        return true;
    }

    auto upper = std::upper_bound(
        node.keys.begin(),
        node.keys.end(),
        timeSeconds,
        [](const double value, const ImportedGltfKey& key) {
            return value < key.timeSeconds;
        }
    );
    if (upper == node.keys.begin() || upper == node.keys.end()) {
        translation = node.keys.front().translation;
        rotation = node.keys.front().rotation;
        return true;
    }

    const ImportedGltfKey& to = *upper;
    const ImportedGltfKey& from = *(upper - 1);
    const double duration = to.timeSeconds - from.timeSeconds;
    const double factor = duration > 0.0
        ? std::clamp((timeSeconds - from.timeSeconds) / duration, 0.0, 1.0)
        : 0.0;

    translation = lerpVec3(from.translation, to.translation, factor);
    rotation = slerpQuaternion(from.rotation, to.rotation, factor);
    return true;
}

} // namespace ovtr
