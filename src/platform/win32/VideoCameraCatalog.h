#pragma once

#include <string>
#include <vector>

namespace ovtr::win32 {

struct VideoCameraInfo {
    std::wstring displayName;
    std::wstring symbolicLink;
};

struct VideoCameraCatalogResult {
    bool success = false;
    std::string error;
    std::vector<VideoCameraInfo> cameras;
};

VideoCameraCatalogResult enumerateVideoCameras();

} // namespace ovtr::win32
