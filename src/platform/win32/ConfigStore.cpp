#include "platform/win32/ConfigStore.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <system_error>
#include <vector>

namespace ovtr::win32 {

std::filesystem::path executableDirectoryPath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        const DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );
        if (length == 0) {
            return std::filesystem::current_path();
        }
        if (length < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
        if (buffer.size() > 32768) {
            return std::filesystem::current_path();
        }
    }
}

std::filesystem::path configDirectoryPath()
{
    return executableDirectoryPath() / "config";
}

std::filesystem::path configFilePath(const char* fileName)
{
    return configDirectoryPath() / fileName;
}

std::filesystem::path legacyExecutableConfigPath(const char* fileName)
{
    return executableDirectoryPath() / fileName;
}

std::filesystem::path legacyWorkingDirectoryConfigPath(const char* fileName)
{
    return std::filesystem::current_path() / fileName;
}

std::filesystem::path readableConfigPath(const char* fileName)
{
    const std::filesystem::path preferredPath = configFilePath(fileName);
    std::error_code error;
    if (std::filesystem::exists(preferredPath, error)) {
        return preferredPath;
    }

    const std::filesystem::path executableLegacyPath = legacyExecutableConfigPath(fileName);
    error.clear();
    if (std::filesystem::exists(executableLegacyPath, error)) {
        return executableLegacyPath;
    }

    const std::filesystem::path workingDirectoryLegacyPath = legacyWorkingDirectoryConfigPath(fileName);
    error.clear();
    if (std::filesystem::exists(workingDirectoryLegacyPath, error)) {
        return workingDirectoryLegacyPath;
    }

    return preferredPath;
}

bool ensureConfigDirectory(std::string& error)
{
    std::error_code createError;
    const std::filesystem::path directory = configDirectoryPath();
    std::filesystem::create_directories(directory, createError);
    if (createError) {
        error = "could not create " + directory.string() + ": " + createError.message();
        return false;
    }
    return true;
}

} // namespace ovtr::win32
