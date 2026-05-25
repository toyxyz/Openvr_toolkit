#include "util/SteamVRRuntime.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <array>
#include <filesystem>
#include <sstream>
#include <vector>

namespace ovtr {

namespace {

#ifdef _WIN32

using VRIsRuntimeInstalledFn = bool (*)();
using VRIsHmdPresentFn = bool (*)();
using VRRuntimePathFn = const char* (*)();

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

std::vector<std::filesystem::path> candidateDllPaths()
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

#endif

} // namespace

SteamVRRuntime::SteamVRRuntime() = default;

SteamVRRuntime::~SteamVRRuntime()
{
    unload();
}

SteamVRRuntimeStatus SteamVRRuntime::queryStatus()
{
    SteamVRRuntimeStatus status;

#ifdef _WIN32
    unload();

    std::string lastError;
    for (const std::filesystem::path& candidate : candidateDllPaths()) {
        HMODULE module = LoadLibraryW(candidate.wstring().c_str());
        if (module != nullptr) {
            library_ = module;
            status.dllLoaded = true;
            status.dllPath = candidate.generic_string();
            break;
        }
        lastError = lastWin32ErrorMessage();
    }

    if (!library_) {
        status.error = "Unable to load openvr_api.dll. " + lastError;
        return status;
    }

    const auto runtimeInstalled = reinterpret_cast<VRIsRuntimeInstalledFn>(
        GetProcAddress(static_cast<HMODULE>(library_), "VR_IsRuntimeInstalled")
    );
    const auto hmdPresent = reinterpret_cast<VRIsHmdPresentFn>(
        GetProcAddress(static_cast<HMODULE>(library_), "VR_IsHmdPresent")
    );
    const auto runtimePath = reinterpret_cast<VRRuntimePathFn>(
        GetProcAddress(static_cast<HMODULE>(library_), "VR_RuntimePath")
    );

    if (!runtimeInstalled || !hmdPresent || !runtimePath) {
        status.error = "openvr_api.dll loaded, but required runtime query exports were not found";
        return status;
    }

    status.runtimeInstalled = runtimeInstalled();
    status.hmdPresent = hmdPresent();

    if (const char* path = runtimePath(); path != nullptr) {
        status.runtimePath = path;
    }

    return status;
#else
    status.error = "SteamVR runtime probing is only implemented on Windows";
    return status;
#endif
}

void SteamVRRuntime::unload()
{
#ifdef _WIN32
    if (library_ != nullptr) {
        FreeLibrary(static_cast<HMODULE>(library_));
        library_ = nullptr;
    }
#endif
}

} // namespace ovtr

