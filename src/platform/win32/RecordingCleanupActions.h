#pragma once

#include <filesystem>
#include <string>
#include <vector>

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
bool deleteTemporarySessionFolders(
    const std::filesystem::path& recordingsRoot,
    std::vector<std::string>& messages
);

} // namespace ovtr::win32
