#pragma once

#ifdef _WIN32

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::detail {

std::string narrowFromWide(const std::wstring& value);
std::string lastWin32ErrorMessage();
std::vector<std::filesystem::path> candidateSteamVRRuntimeDllPaths();

} // namespace ovtr::detail

#endif
