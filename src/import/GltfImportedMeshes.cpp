#include "import/GltfImportedSceneParts.h"

#include "import/GltfJsonUtils.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace ovtr {

bool parseImportedGltfMeshes(
    const JsonValue& root,
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    ImportedGltfScene& scene,
    std::string& error
)
{
    const JsonValue* meshesValue = root.find("meshes");
    if (!meshesValue || meshesValue->type != JsonValue::Type::Array) {
        return true;
    }

    scene.meshes.resize(meshesValue->arrayValue.size());
    for (std::size_t meshIndex = 0; meshIndex < meshesValue->arrayValue.size(); ++meshIndex) {
        const JsonValue& meshValue = meshesValue->arrayValue[meshIndex];
        if (meshValue.type != JsonValue::Type::Object) {
            error = "glTF mesh must be an object";
            return false;
        }

        const JsonValue* primitivesValue = meshValue.find("primitives");
        if (!primitivesValue || primitivesValue->type != JsonValue::Type::Array ||
            primitivesValue->arrayValue.empty()) {
            continue;
        }

        const JsonValue& primitive = primitivesValue->arrayValue.front();
        if (primitive.type != JsonValue::Type::Object) {
            error = "glTF mesh primitive must be an object";
            return false;
        }

        const JsonValue* attributes = primitive.find("attributes");
        if (!attributes || attributes->type != JsonValue::Type::Object) {
            continue;
        }

        const int positionAccessor = jsonIntMember(*attributes, "POSITION", -1);
        const int normalAccessor = jsonIntMember(*attributes, "NORMAL", -1);
        const int indexAccessor = jsonIntMember(primitive, "indices", -1);
        if (positionAccessor < 0 || indexAccessor < 0) {
            continue;
        }

        std::vector<float> positions;
        if (!readGltfAccessorFloats(bufferViews, accessors, positionAccessor, 3, binary, positions, error)) {
            return false;
        }
        std::vector<float> normals;
        if (normalAccessor >= 0 &&
            !readGltfAccessorFloats(bufferViews, accessors, normalAccessor, 3, binary, normals, error)) {
            return false;
        }
        std::vector<std::uint32_t> indices;
        if (!readGltfAccessorIndices(bufferViews, accessors, indexAccessor, binary, indices, error)) {
            return false;
        }

        const std::size_t vertexCount = positions.size() / 3;
        RenderModelGeometry geometry;
        geometry.vertices.reserve(vertexCount);
        for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            RenderModelVertex vertex;
            vertex.position = {
                positions[vertexIndex * 3],
                positions[vertexIndex * 3 + 1],
                positions[vertexIndex * 3 + 2],
            };
            if (normals.size() >= vertexIndex * 3 + 3) {
                vertex.normal = {
                    normals[vertexIndex * 3],
                    normals[vertexIndex * 3 + 1],
                    normals[vertexIndex * 3 + 2],
                };
            }
            geometry.vertices.push_back(vertex);
        }
        geometry.indices = std::move(indices);
        geometry.available = !geometry.vertices.empty() && !geometry.indices.empty();
        scene.meshes[meshIndex] = std::move(geometry);
    }
    return true;
}

} // namespace ovtr
