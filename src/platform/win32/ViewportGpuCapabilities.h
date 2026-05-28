#pragma once

#include <string>

namespace ovtr::win32 {

struct ViewportGpuCapabilities {
    bool gladLoaded = false;
    bool shaderVboAvailable = false;
    std::string failureReason;
};

} // namespace ovtr::win32
