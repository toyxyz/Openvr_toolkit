#include "platform/win32/ConfigStore.h"

#include <system_error>

namespace ovtr::win32 {

std::filesystem::path defaultExportDirectoryPath()
{
    return std::filesystem::current_path() / "exports";
}

std::filesystem::path normalizedExportDirectoryPath(const std::filesystem::path& path)
{
    const std::filesystem::path requested = path.empty()
        ? defaultExportDirectoryPath()
        : path;
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

} // namespace ovtr::win32
