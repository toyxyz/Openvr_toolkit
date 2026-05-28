#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ovtr {

struct GltfGlbPayload {
    std::string json;
    std::vector<std::uint8_t> binary;
};

bool readGltfGlbPayload(
    const std::filesystem::path& path,
    GltfGlbPayload& payload,
    std::string& error
);

} // namespace ovtr
