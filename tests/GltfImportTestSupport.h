#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::test {

void writeTestGlb(
    const std::filesystem::path& path,
    const std::string& json,
    const std::vector<std::uint8_t>& binary,
    bool includeJson,
    bool includeBinary
);

std::string minimalGltfJson(const std::string& accessors, const std::string& meshes);

} // namespace ovtr::test
