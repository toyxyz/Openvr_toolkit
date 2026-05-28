#include "export/GltfExportBuffers.h"

#include "util/BinaryBuffer.h"

#include <utility>

namespace ovtr {

int appendGltfFloatBufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<float>& values,
    const int target
)
{
    padToAlignment(buffer);
    const GltfExportBufferView view{
        buffer.size(),
        values.size() * sizeof(float),
        target,
    };
    for (const float value : values) {
        appendLittleEndianFloat32(buffer, value);
    }
    bufferViews.push_back(view);
    return static_cast<int>(bufferViews.size() - 1);
}

int appendGltfUint16BufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<std::uint16_t>& values,
    const int target
)
{
    padToAlignment(buffer);
    const GltfExportBufferView view{
        buffer.size(),
        values.size() * sizeof(std::uint16_t),
        target,
    };
    for (const std::uint16_t value : values) {
        appendLittleEndianUint16(buffer, value);
    }
    bufferViews.push_back(view);
    return static_cast<int>(bufferViews.size() - 1);
}

int addGltfAccessor(
    std::vector<GltfExportAccessor>& accessors,
    const int bufferView,
    const int componentType,
    const int count,
    std::string type,
    std::vector<double> minValues,
    std::vector<double> maxValues
)
{
    accessors.push_back({
        bufferView,
        componentType,
        count,
        std::move(type),
        std::move(minValues),
        std::move(maxValues),
    });
    return static_cast<int>(accessors.size() - 1);
}

} // namespace ovtr
