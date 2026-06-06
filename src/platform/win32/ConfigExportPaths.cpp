#include "platform/win32/ConfigStore.h"

#include <system_error>

namespace ovtr::win32 {
namespace {

std::filesystem::path normalizedDirectoryPath(
    const std::filesystem::path& path,
    const std::filesystem::path& defaultPath
)
{
    const std::filesystem::path requested = path.empty() ? defaultPath : path;
    if (requested.is_absolute()) {
        return requested.lexically_normal();
    }

    std::error_code error;
    const std::filesystem::path absolutePath = std::filesystem::absolute(requested, error);
    if (!error) {
        return absolutePath.lexically_normal();
    }
    return (std::filesystem::current_path() / requested).lexically_normal();
}

} // namespace

std::filesystem::path defaultExportDirectoryPath()
{
    return std::filesystem::current_path() / "exports";
}

std::filesystem::path normalizedExportDirectoryPath(const std::filesystem::path& path)
{
    return normalizedDirectoryPath(path, defaultExportDirectoryPath());
}

std::filesystem::path defaultSessionDirectoryPath()
{
    return std::filesystem::current_path() / "recordings";
}

std::filesystem::path normalizedSessionDirectoryPath(const std::filesystem::path& path)
{
    return normalizedDirectoryPath(path, defaultSessionDirectoryPath());
}

} // namespace ovtr::win32
