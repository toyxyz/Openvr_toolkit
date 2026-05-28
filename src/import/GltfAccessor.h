#pragma once

#include "import/GltfJson.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

struct GltfBufferView {
    int buffer = 0;
    std::size_t byteOffset = 0;
    std::size_t byteLength = 0;
    std::size_t byteStride = 0;
};

struct GltfAccessor {
    int bufferView = -1;
    std::size_t byteOffset = 0;
    int componentType = 0;
    int count = 0;
    std::string type;
};

bool parseGltfBufferViews(
    const JsonValue& root,
    std::vector<GltfBufferView>& bufferViews,
    std::string& error
);

bool parseGltfAccessors(
    const JsonValue& root,
    std::vector<GltfAccessor>& accessors,
    std::string& error
);

bool readGltfAccessorFloats(
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    int accessorIndex,
    int expectedComponentCount,
    const std::vector<std::uint8_t>& binary,
    std::vector<float>& output,
    std::string& error
);

bool readGltfAccessorIndices(
    const std::vector<GltfBufferView>& bufferViews,
    const std::vector<GltfAccessor>& accessors,
    int accessorIndex,
    const std::vector<std::uint8_t>& binary,
    std::vector<std::uint16_t>& output,
    std::string& error
);

} // namespace ovtr
