#pragma once

#include <cstddef>
#include <string>

namespace ovtr {

inline constexpr int kGltfComponentByte = 5120;
inline constexpr int kGltfComponentUnsignedByte = 5121;
inline constexpr int kGltfComponentShort = 5122;
inline constexpr int kGltfComponentUnsignedShort = 5123;
inline constexpr int kGltfComponentUnsignedInt = 5125;
inline constexpr int kGltfComponentFloat = 5126;

int gltfAccessorComponentCount(const std::string& type);
std::size_t gltfAccessorComponentByteSize(int componentType);

} // namespace ovtr
