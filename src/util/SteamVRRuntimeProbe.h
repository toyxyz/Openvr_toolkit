#pragma once

#include "util/SteamVRRuntime.h"

namespace ovtr::detail {

#ifdef _WIN32
SteamVRRuntimeStatus querySteamVRRuntimeStatus(void*& library);
void unloadSteamVRRuntimeLibrary(void*& library);
#endif

} // namespace ovtr::detail
