#pragma once

#include "import/GltfAccessor.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

struct GltfAccessorReadLayout {
    const GltfAccessor* accessor = nullptr;
    std::size_t baseOffset = 0;
    std::size_t stride = 0;
};

bool resolveGltfAccessorReadLayout(
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    int accessorIndex,
    int expectedComponentCount,
    const std::vector<std::uint8_t>& binary,
    GltfAccessorReadLayout& layout,
    std::string& error
);

} // namespace ovtr
