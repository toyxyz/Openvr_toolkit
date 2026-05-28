#include "util/SteamVRRuntime.h"

#include "util/SteamVRRuntimeProbe.h"

namespace ovtr {

SteamVRRuntime::SteamVRRuntime() = default;

SteamVRRuntime::~SteamVRRuntime()
{
    unload();
}

SteamVRRuntimeStatus SteamVRRuntime::queryStatus()
{
#ifdef _WIN32
    return detail::querySteamVRRuntimeStatus(library_);
#else
    SteamVRRuntimeStatus status;
    status.error = "SteamVR runtime probing is only implemented on Windows";
    return status;
#endif
}

void SteamVRRuntime::unload()
{
#ifdef _WIN32
    detail::unloadSteamVRRuntimeLibrary(library_);
#else
    library_ = nullptr;
#endif
}

} // namespace ovtr
