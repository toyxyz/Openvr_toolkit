#include "util/SteamVRRuntimeProbe.h"
#include "util/SteamVRRuntimeProbeWin32Internal.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ovtr::detail {
namespace {

using VRIsRuntimeInstalledFn = bool (*)();
using VRIsHmdPresentFn = bool (*)();
using VRRuntimePathFn = const char* (*)();

} // namespace

SteamVRRuntimeStatus querySteamVRRuntimeStatus(void*& library)
{
    unloadSteamVRRuntimeLibrary(library);

    SteamVRRuntimeStatus status;
    std::string lastError;
    for (const std::filesystem::path& candidate : candidateSteamVRRuntimeDllPaths()) {
        HMODULE module = LoadLibraryW(candidate.wstring().c_str());
        if (module != nullptr) {
            library = module;
            status.dllLoaded = true;
            status.dllPath = candidate.generic_string();
            break;
        }
        lastError = lastWin32ErrorMessage();
    }

    if (library == nullptr) {
        status.error = "Unable to load openvr_api.dll. " + lastError;
        return status;
    }

    const auto runtimeInstalled = reinterpret_cast<VRIsRuntimeInstalledFn>(
        GetProcAddress(static_cast<HMODULE>(library), "VR_IsRuntimeInstalled")
    );
    const auto hmdPresent = reinterpret_cast<VRIsHmdPresentFn>(
        GetProcAddress(static_cast<HMODULE>(library), "VR_IsHmdPresent")
    );
    const auto runtimePath = reinterpret_cast<VRRuntimePathFn>(
        GetProcAddress(static_cast<HMODULE>(library), "VR_RuntimePath")
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
}

void unloadSteamVRRuntimeLibrary(void*& library)
{
    if (library != nullptr) {
        FreeLibrary(static_cast<HMODULE>(library));
        library = nullptr;
    }
}

} // namespace ovtr::detail
#endif
