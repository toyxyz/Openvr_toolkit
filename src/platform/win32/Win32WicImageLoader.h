#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace ovtr::win32 {

struct WicRgbaImage {
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector<std::uint8_t> pixels;
};

bool loadRgbaImageWithWic(const std::filesystem::path& path, WicRgbaImage& image) noexcept;

} // namespace ovtr::win32
