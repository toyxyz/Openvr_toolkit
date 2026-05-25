#pragma once

#include <filesystem>
#include <string>

namespace ovtr {

struct ExportResult {
    bool success = false;
    std::filesystem::path outputPath;
    std::string error;
};

} // namespace ovtr
