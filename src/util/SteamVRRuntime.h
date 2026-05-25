#pragma once

#include <string>

namespace ovtr {

struct SteamVRRuntimeStatus {
    bool dllLoaded = false;
    bool runtimeInstalled = false;
    bool hmdPresent = false;
    std::string dllPath;
    std::string runtimePath;
    std::string error;
};

class SteamVRRuntime {
public:
    SteamVRRuntime();
    ~SteamVRRuntime();

    SteamVRRuntime(const SteamVRRuntime&) = delete;
    SteamVRRuntime& operator=(const SteamVRRuntime&) = delete;

    SteamVRRuntimeStatus queryStatus();

private:
    void unload();

    void* library_ = nullptr;
};

} // namespace ovtr

