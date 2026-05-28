#include "util/SteamVRRuntimeProbeWin32Internal.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <sstream>

namespace ovtr::detail {

std::string narrowFromWide(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (required <= 0) {
        return {};
    }

    std::string output(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required,
        nullptr,
        nullptr
    );
    return output;
}

std::string lastWin32ErrorMessage()
{
    const DWORD error = GetLastError();
    if (error == 0) {
        return {};
    }

    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );

    std::wstring message;
    if (length > 0 && buffer != nullptr) {
        message.assign(buffer, length);
    }

    if (buffer != nullptr) {
        LocalFree(buffer);
    }

    std::ostringstream stream;
    stream << "Win32 error " << error;
    const std::string text = narrowFromWide(message);
    if (!text.empty()) {
        stream << ": " << text;
    }
    return stream.str();
}

std::vector<std::filesystem::path> candidateSteamVRRuntimeDllPaths()
{
    std::vector<std::filesystem::path> paths;
    paths.emplace_back("openvr_api.dll");

#if defined(_M_X64) || defined(__x86_64__)
    paths.emplace_back("C:/Program Files (x86)/Steam/steamapps/common/SteamVR/bin/win64/openvr_api.dll");
#else
    paths.emplace_back("C:/Program Files (x86)/Steam/steamapps/common/SteamVR/bin/win32/openvr_api.dll");
#endif

    return paths;
}

} // namespace ovtr::detail
#endif
