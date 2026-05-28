#include "export/GltfExportSceneParts.h"

#include <array>
#include <vector>

namespace ovtr {
namespace {

std::vector<double> boundsValues(const std::array<double, 3>& bounds)
{
    return {bounds[0], bounds[1], bounds[2]};
}

} // namespace

void buildGltfMeshData(
    std::vector<GltfDevice>& devices,
    std::vector<std::uint8_t>& binary,
    std::vector<GltfExportBufferView>& bufferViews,
    std::vector<GltfExportAccessor>& accessors,
    std::vector<GltfMeshPrimitive>& meshes
)
{
    for (GltfDevice& device : devices) {
        if (!device.geometry.available || device.geometry.vertices.empty() || device.geometry.indices.empty()) {
            continue;
        }

        std::vector<float> positions;
        std::vector<float> normals;
        positions.reserve(device.geometry.vertices.size() * 3);
        normals.reserve(device.geometry.vertices.size() * 3);
        for (const RenderModelVertex& vertex : device.geometry.vertices) {
            positions.push_back(vertex.position[0]);
            positions.push_back(vertex.position[1]);
            positions.push_back(vertex.position[2]);

            normals.push_back(vertex.normal[0]);
            normals.push_back(vertex.normal[1]);
            normals.push_back(vertex.normal[2]);
        }

        const int positionView = appendGltfFloatBufferView(binary, bufferViews, positions, kGltfArrayBuffer);
        const int normalView = appendGltfFloatBufferView(binary, bufferViews, normals, kGltfArrayBuffer);
        const GltfExportIndexBufferView indexView = appendGltfIndexBufferView(
            binary,
            bufferViews,
            device.geometry.indices,
            kGltfElementArrayBuffer
        );
        const RenderModelPositionBounds positionBounds = renderModelPositionBounds(device.geometry);

        const int positionAccessor = addGltfAccessor(
            accessors,
            positionView,
            kGltfComponentFloat,
            static_cast<int>(device.geometry.vertices.size()),
            "VEC3",
            positionBounds.valid ? boundsValues(positionBounds.min) : std::vector<double>{},
            positionBounds.valid ? boundsValues(positionBounds.max) : std::vector<double>{}
        );
        const int normalAccessor = addGltfAccessor(
            accessors,
            normalView,
            kGltfComponentFloat,
            static_cast<int>(device.geometry.vertices.size()),
            "VEC3"
        );
        const int indexAccessor = addGltfAccessor(
            accessors,
            indexView.bufferView,
            indexView.componentType,
            static_cast<int>(device.geometry.indices.size()),
            "SCALAR"
        );

        device.meshIndex = static_cast<int>(meshes.size());
        meshes.push_back({
            device.nodeName + "_Mesh",
            positionAccessor,
            normalAccessor,
            indexAccessor,
        });
    }
}

} // namespace ovtr
