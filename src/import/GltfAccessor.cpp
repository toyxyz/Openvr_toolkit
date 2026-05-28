#include "import/GltfAccessor.h"

#include "import/GltfAccessorLayout.h"
#include "import/GltfJsonUtils.h"

#include <utility>

namespace ovtr {

bool parseGltfBufferViews(
    const JsonValue& root,
    std::vector<GltfBufferView>& bufferViews,
    std::string& error
)
{
    const JsonValue* bufferViewsValue = root.find("bufferViews");
    if (!bufferViewsValue || bufferViewsValue->type != JsonValue::Type::Array) {
        error = "glTF bufferViews are missing";
        return false;
    }

    bufferViews.clear();
    bufferViews.reserve(bufferViewsValue->arrayValue.size());
    for (const JsonValue& value : bufferViewsValue->arrayValue) {
        if (value.type != JsonValue::Type::Object) {
            error = "glTF bufferView must be an object";
            return false;
        }
        GltfBufferView view;
        view.buffer = jsonIntMember(value, "buffer", 0);
        view.byteOffset = jsonSizeMember(value, "byteOffset", 0);
        view.byteLength = jsonSizeMember(value, "byteLength", 0);
        view.byteStride = jsonSizeMember(value, "byteStride", 0);
        if (view.buffer != 0) {
            error = "only embedded GLB buffer 0 is supported";
            return false;
        }
        bufferViews.push_back(view);
    }
    return true;
}

bool parseGltfAccessors(
    const JsonValue& root,
    std::vector<GltfAccessor>& accessors,
    std::string& error
)
{
    const JsonValue* accessorsValue = root.find("accessors");
    if (!accessorsValue || accessorsValue->type != JsonValue::Type::Array) {
        error = "glTF accessors are missing";
        return false;
    }

    accessors.clear();
    accessors.reserve(accessorsValue->arrayValue.size());
    for (const JsonValue& value : accessorsValue->arrayValue) {
        if (value.type != JsonValue::Type::Object) {
            error = "glTF accessor must be an object";
            return false;
        }

        GltfAccessor accessor;
        accessor.bufferView = jsonIntMember(value, "bufferView", -1);
        accessor.byteOffset = jsonSizeMember(value, "byteOffset", 0);
        accessor.componentType = jsonIntMember(value, "componentType", 0);
        accessor.count = jsonIntMember(value, "count", 0);
        accessor.type = jsonStringMember(value, "type");
        if (accessor.bufferView < 0 || accessor.count < 0 ||
            gltfAccessorComponentCount(accessor.type) <= 0 ||
            gltfAccessorComponentByteSize(accessor.componentType) == 0) {
            error = "glTF accessor has unsupported layout";
            return false;
        }
        accessors.push_back(std::move(accessor));
    }
    return true;
}

} // namespace ovtr
