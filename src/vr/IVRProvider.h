#pragma once

#include "data/SessionTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ovtr {

enum class VRConnectionState {
    RuntimeNotInstalled,
    SteamVRNotRunning,
    HmdNotPresent,
    InitFailed,
    Connected,
    ConnectionLost,
};

enum class VREventType {
    DeviceActivated,
    DeviceDeactivated,
    DeviceUpdated,
    Quit,
    Other,
};

struct VREvent {
    VREventType type = VREventType::Other;
    std::uint32_t runtimeIndex = 0;
};

struct PosePollResult {
    std::uint64_t timestampNs = 0;
    std::vector<PoseSample> poses;
};

class IVRProvider {
public:
    virtual ~IVRProvider() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
    virtual VRConnectionState connectionState() const = 0;
    virtual std::string lastError() const = 0;

    virtual bool pollEvents(std::vector<VREvent>& outEvents) = 0;
    virtual bool pollPoses(PosePollResult& outResult) = 0;
    virtual std::vector<DeviceDescriptor> enumerateDevices() const = 0;
};

} // namespace ovtr

