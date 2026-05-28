#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

constexpr int kGltfComponentFloat = 5126;
constexpr int kGltfComponentUnsignedShort = 5123;
constexpr int kGltfComponentUnsignedInt = 5125;
constexpr int kGltfArrayBuffer = 34962;
constexpr int kGltfElementArrayBuffer = 34963;

struct GltfExportBufferView {
    std::size_t byteOffset = 0;
    std::size_t byteLength = 0;
    int target = 0;
};

struct GltfExportAccessor {
    int bufferView = -1;
    int componentType = kGltfComponentFloat;
    int count = 0;
    std::string type;
    std::vector<double> minValues;
    std::vector<double> maxValues;
};

struct GltfExportIndexBufferView {
    int bufferView = -1;
    int componentType = kGltfComponentUnsignedShort;
};

int appendGltfFloatBufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<float>& values,
    int target = 0
);

GltfExportIndexBufferView appendGltfIndexBufferView(
    std::vector<std::uint8_t>& buffer,
    std::vector<GltfExportBufferView>& bufferViews,
    const std::vector<std::uint32_t>& values,
    int target = 0
);

int addGltfAccessor(
    std::vector<GltfExportAccessor>& accessors,
    int bufferView,
    int componentType,
    int count,
    std::string type,
    std::vector<double> minValues = {},
    std::vector<double> maxValues = {}
);

} // namespace ovtr
