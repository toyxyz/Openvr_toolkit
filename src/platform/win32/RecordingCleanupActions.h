#pragma once

#include <filesystem>
#include <string>

namespace ovtr::win32 {

bool isPathWithinDirectory(
    const std::filesystem::path& child,
    const std::filesystem::path& parent
);
bool deleteTemporarySessionFolder(
    std::filesystem::path& currentSessionFolder,
    const std::filesystem::path& recordingsRoot,
    std::string& message
);

} // namespace ovtr::win32
