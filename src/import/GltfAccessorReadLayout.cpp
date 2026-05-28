#include "import/GltfAccessorReadLayout.h"

#include "import/GltfAccessorLayout.h"

#include <limits>

namespace ovtr {
namespace {

bool checkedAddSize(const std::size_t left, const std::size_t right, std::size_t& output) noexcept
{
    if (left > std::numeric_limits<std::size_t>::max() - right) {
        return false;
    }

    output = left + right;
    return true;
}

bool checkedMultiplySize(const std::size_t left, const std::size_t right, std::size_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left) {
        return false;
    }

    output = left * right;
    return true;
}

} // namespace

bool resolveGltfAccessorReadLayout(
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    const int accessorIndex,
    const int expectedComponentCount,
    const std::vector<std::uint8_t>& binary,
    GltfAccessorReadLayout& layout,
    std::string& error
)
{
    layout = {};
    if (accessorIndex < 0 || static_cast<std::size_t>(accessorIndex) >= accessors.size()) {
        error = "glTF accessor index is out of range";
        return false;
    }

    layout.accessor = &accessors[static_cast<std::size_t>(accessorIndex)];
    if (layout.accessor->bufferView < 0 ||
        static_cast<std::size_t>(layout.accessor->bufferView) >= bufferViews.size()) {
        error = "glTF accessor bufferView is out of range";
        return false;
    }
    const GltfBufferView& view = bufferViews[static_cast<std::size_t>(layout.accessor->bufferView)];

    const int componentCount = gltfAccessorComponentCount(layout.accessor->type);
    if (componentCount != expectedComponentCount) {
        error = "glTF accessor type does not match expected data";
        return false;
    }

    const std::size_t componentSize = gltfAccessorComponentByteSize(layout.accessor->componentType);
    std::size_t elementByteSize = 0;
    if (!checkedMultiplySize(componentSize, static_cast<std::size_t>(componentCount), elementByteSize)) {
        error = "glTF accessor layout size overflow";
        return false;
    }

    layout.stride = view.byteStride != 0
        ? view.byteStride
        : elementByteSize;
    if (layout.stride < elementByteSize) {
        error = "glTF accessor byteStride is smaller than element size";
        return false;
    }

    std::size_t viewEnd = 0;
    if (!checkedAddSize(view.byteOffset, view.byteLength, viewEnd)) {
        error = "glTF bufferView range overflows";
        return false;
    }

    if (!checkedAddSize(view.byteOffset, layout.accessor->byteOffset, layout.baseOffset)) {
        error = "glTF accessor byte offset overflows";
        return false;
    }
    if (layout.accessor->count == 0) {
        return true;
    }

    std::size_t lastElementDelta = 0;
    if (!checkedMultiplySize(
            layout.stride,
            static_cast<std::size_t>(layout.accessor->count - 1),
            lastElementDelta
        )) {
        error = "glTF accessor range overflows";
        return false;
    }

    std::size_t lastElementOffset = 0;
    if (!checkedAddSize(layout.baseOffset, lastElementDelta, lastElementOffset)) {
        error = "glTF accessor range overflows";
        return false;
    }

    std::size_t requiredEnd = 0;
    if (!checkedAddSize(lastElementOffset, elementByteSize, requiredEnd)) {
        error = "glTF accessor range overflows";
        return false;
    }
    if (requiredEnd > binary.size() || requiredEnd > viewEnd) {
        error = "glTF accessor reads past its bufferView";
        return false;
    }
    return true;
}

} // namespace ovtr
