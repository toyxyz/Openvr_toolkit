#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {

bool writeGltfTextFile(const std::filesystem::path& path, const std::string& text);
bool writeGltfBinaryFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& binary);
bool writeGltfGlbFile(
    const std::filesystem::path& path,
    const std::string& json,
    const std::vector<std::uint8_t>& binary
);
std::filesystem::path gltfSiblingBinPath(const std::filesystem::path& gltfPath);

} // namespace ovtr
