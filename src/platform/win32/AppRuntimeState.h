#pragma once

#include "util/SteamVRRuntime.h"
#include "vr/OpenVRProvider.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppRuntimeState {
    SteamVRRuntime runtime;
    SteamVRRuntimeStatus status;
    OpenVRProvider provider;
    std::vector<DeviceDescriptor> devices;
    PosePollResult poses;
    std::string providerError;
    std::string lastLoggedProviderError;
    std::size_t lastLoggedDeviceCount = 0;
    std::chrono::steady_clock::time_point lastFpsUpdate = std::chrono::steady_clock::now();
    std::atomic_uint64_t posePollFrames = 0;
    std::uint64_t renderFrames = 0;
    double posePollFps = 0.0;
    double renderFps = 0.0;
    std::chrono::steady_clock::time_point lastDeviceEnumeration{};
};

} // namespace ovtr::win32
