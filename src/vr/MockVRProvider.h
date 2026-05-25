#pragma once

#include "vr/IVRProvider.h"

#include <vector>

namespace ovtr {

class MockVRProvider final : public IVRProvider {
public:
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;
    VRConnectionState connectionState() const override;
    std::string lastError() const override;

    bool pollEvents(std::vector<VREvent>& outEvents) override;
    bool pollPoses(PosePollResult& outResult) override;
    std::vector<DeviceDescriptor> enumerateDevices() const override;

    void setDevices(std::vector<DeviceDescriptor> devices);
    void setPoseResult(PosePollResult result);
    void pushEvent(VREvent event);

private:
    bool initialized_ = false;
    VRConnectionState state_ = VRConnectionState::SteamVRNotRunning;
    std::vector<DeviceDescriptor> devices_;
    PosePollResult poseResult_;
    std::vector<VREvent> pendingEvents_;
};

} // namespace ovtr

