#include "import/GltfAccessor.h"

#include "import/GltfAccessorLayout.h"
#include "import/GltfAccessorReadLayout.h"

#include <cstring>

namespace ovtr {
namespace {

float readFloatComponent(const std::vector<std::uint8_t>& binary, const std::size_t offset)
{
    float value = 0.0f;
    std::memcpy(&value, binary.data() + offset, sizeof(float));
    return value;
}

} // namespace

bool readGltfAccessorFloats(
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const int accessorIndex,
    const int expectedComponentCount,
    const std::vector<std::uint8_t>& binary,
    std::vector<float>& output,
    std::string& error
)
{
    GltfAccessorReadLayout layout;
    if (!resolveGltfAccessorReadLayout(
            bufferViews,
            accessors,
            accessorIndex,
            expectedComponentCount,
            binary,
            layout,
            error
        )) {
        return false;
    }

    if (layout.accessor->componentType != kGltfComponentFloat) {
        error = "only float accessors are supported for transforms and vertices";
        return false;
    }

    output.clear();
    output.reserve(static_cast<std::size_t>(layout.accessor->count) * static_cast<std::size_t>(expectedComponentCount));
    for (int element = 0; element < layout.accessor->count; ++element) {
        const std::size_t elementOffset = layout.baseOffset + layout.stride * static_cast<std::size_t>(element);
        for (int component = 0; component < expectedComponentCount; ++component) {
            output.push_back(readFloatComponent(
                binary,
                elementOffset + sizeof(float) * static_cast<std::size_t>(component)
            ));
        }
    }
    return true;
}

} // namespace ovtr
