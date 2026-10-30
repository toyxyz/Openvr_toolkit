#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ovtr::win32 {

struct VideoFrame {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t stride = 0;
    std::uint64_t sequence = 0;
    std::int64_t timestamp100ns = 0;
    bool topDown = true;
    std::vector<std::uint8_t> bgra;
};

using VideoFrameSnapshot = std::shared_ptr<const VideoFrame>;

} // namespace ovtr::win32
