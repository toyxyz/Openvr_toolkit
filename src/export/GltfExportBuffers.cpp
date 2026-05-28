#include "export/GltfExportBuffers.h"

#include "util/BinaryBuffer.h"

#include <limits>
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

GltfExportIndexBufferView appendGltfIndexBufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<std::uint32_t>& values,
    const int target
)
{
    bool needsUint32 = false;
    for (const std::uint32_t value : values) {
        if (value > std::numeric_limits<std::uint16_t>::max()) {
            needsUint32 = true;
            break;
        }
    }

    padToAlignment(buffer);
    const GltfExportBufferView view{
        buffer.size(),
        values.size() * (needsUint32 ? sizeof(std::uint32_t) : sizeof(std::uint16_t)),
        target,
    };
    for (const std::uint32_t value : values) {
        if (needsUint32) {
            appendLittleEndianUint32(buffer, value);
        } else {
            appendLittleEndianUint16(buffer, static_cast<std::uint16_t>(value));
        }
    }
    bufferViews.push_back(view);
    return {
        static_cast<int>(bufferViews.size() - 1),
        needsUint32 ? kGltfComponentUnsignedInt : kGltfComponentUnsignedShort,
    };
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
