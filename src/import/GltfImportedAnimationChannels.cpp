#include "import/GltfImportedAnimationBuilders.h"

#include "import/GltfJsonUtils.h"
#include "math/QuaternionUtils.h"

namespace ovtr::detail {

bool appendGltfAnimationChannel(
    const JsonValue& channel,
    const JsonValue& samplers,
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    std::vector<NodeAnimationBuilder>& builders,
    std::string& error
)
{
    if (channel.type != JsonValue::Type::Object) {
        error = "glTF animation channel must be an object";
        return false;
    }

    const JsonValue* target = channel.find("target");
    if (!target || target->type != JsonValue::Type::Object) {
        error = "glTF animation channel target is missing";
        return false;
    }

    const int nodeIndex = jsonIntMember(*target, "node", -1);
    const std::string path = jsonStringMember(*target, "path");
    const int samplerIndex = jsonIntMember(channel, "sampler", -1);
    if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= builders.size() ||
        samplerIndex < 0 || static_cast<std::size_t>(samplerIndex) >= samplers.arrayValue.size()) {
        return true;
    }
    if (path != "translation" && path != "rotation") {
        return true;
    }

    const JsonValue& sampler = samplers.arrayValue[static_cast<std::size_t>(samplerIndex)];
    if (sampler.type != JsonValue::Type::Object) {
        error = "glTF animation sampler must be an object";
        return false;
    }

    const int inputAccessor = jsonIntMember(sampler, "input", -1);
    const int outputAccessor = jsonIntMember(sampler, "output", -1);
    std::vector<float> times;
    if (!readGltfAccessorFloats(bufferViews, accessors, inputAccessor, 1, binary, times, error)) {
        return false;
    }

    const int componentCount = path == "rotation" ? 4 : 3;
    std::vector<float> values;
    if (!readGltfAccessorFloats(bufferViews, accessors, outputAccessor, componentCount, binary, values, error)) {
        return false;
    }
    if (values.size() < times.size() * static_cast<std::size_t>(componentCount)) {
        error = "glTF animation sampler output has too few values";
        return false;
    }

    NodeAnimationBuilder& builder = builders[static_cast<std::size_t>(nodeIndex)];
    for (std::size_t keyIndex = 0; keyIndex < times.size(); ++keyIndex) {
        NodeAnimationValue& value = builder[static_cast<double>(times[keyIndex])];
        if (path == "translation") {
            value.hasTranslation = true;
            value.translation = {
                values[keyIndex * 3],
                values[keyIndex * 3 + 1],
                values[keyIndex * 3 + 2],
            };
        } else {
            value.hasRotation = true;
            value.rotation = normalizeQuaternion({
                values[keyIndex * 4],
                values[keyIndex * 4 + 1],
                values[keyIndex * 4 + 2],
                values[keyIndex * 4 + 3],
            });
        }
    }
    return true;
}

} // namespace ovtr::detail
