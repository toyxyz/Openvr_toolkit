#pragma once

#include <filesystem>
#include <string>

namespace ovtr {

bool removeVideoReferenceFromSessionManifest(
    const std::filesystem::path& manifestPath,
    std::string& outError
);

} // namespace ovtr
