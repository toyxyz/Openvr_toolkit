#include "import/GltfAccessor.h"

#include "import/GltfAccessorLayout.h"
#include "import/GltfAccessorReadLayout.h"
#include "util/BinaryBuffer.h"

namespace ovtr {
namespace {

std::uint32_t readUnsignedComponent(
    const std::vector<std::uint8_t>& binary,
    const std::size_t offset,
    const int componentType
)
{
    if (componentType == kGltfComponentUnsignedByte) {
        return binary[offset];
    }
    if (componentType == kGltfComponentUnsignedShort) {
        return static_cast<std::uint32_t>(binary[offset]) |
            (static_cast<std::uint32_t>(binary[offset + 1]) << 8u);
    }
    if (componentType == kGltfComponentUnsignedInt) {
        std::uint32_t value = 0;
        readLittleEndianUint32(binary, offset, value);
        return value;
    }
    return 0;
}

} // namespace

bool readGltfAccessorIndices(
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const int accessorIndex,
    const std::vector<std::uint8_t>& binary,
    std::vector<std::uint32_t>& output,
    std::string& error
)
{
    GltfAccessorReadLayout layout;
    if (!resolveGltfAccessorReadLayout(
            bufferViews,
            accessors,
            accessorIndex,
            1,
            binary,
            layout,
            error
        )) {
        return false;
    }

    if (layout.accessor->componentType != kGltfComponentUnsignedByte &&
        layout.accessor->componentType != kGltfComponentUnsignedShort &&
        layout.accessor->componentType != kGltfComponentUnsignedInt) {
        error = "glTF mesh indices must be unsigned integer data";
        return false;
    }

    output.clear();
    output.reserve(static_cast<std::size_t>(layout.accessor->count));
    for (int element = 0; element < layout.accessor->count; ++element) {
        const std::uint32_t index = readUnsignedComponent(
            binary,
            layout.baseOffset + layout.stride * static_cast<std::size_t>(element),
            layout.accessor->componentType
        );
        output.push_back(index);
    }
    return true;
}

} // namespace ovtr
