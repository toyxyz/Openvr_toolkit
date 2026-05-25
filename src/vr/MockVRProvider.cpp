#include "vr/MockVRProvider.h"

#include <utility>

namespace ovtr {

bool MockVRProvider::initialize()
{
    initialized_ = true;
    state_ = VRConnectionState::Connected;
    return true;
}

void MockVRProvider::shutdown()
{
    initialized_ = false;
    state_ = VRConnectionState::SteamVRNotRunning;
}

bool MockVRProvider::isInitialized() const
{
    return initialized_;
}

VRConnectionState MockVRProvider::connectionState() const
{
    return state_;
}

std::string MockVRProvider::lastError() const
{
    return {};
}

bool MockVRProvider::pollEvents(std::vector<VREvent>& outEvents)
{
    outEvents = pendingEvents_;
    pendingEvents_.clear();
    return true;
}

bool MockVRProvider::pollPoses(PosePollResult& outResult)
{
    outResult = poseResult_;
    return initialized_;
}

std::vector<DeviceDescriptor> MockVRProvider::enumerateDevices() const
{
    return devices_;
}

void MockVRProvider::setDevices(std::vector<DeviceDescriptor> devices)
{
    devices_ = std::move(devices);
}

void MockVRProvider::setPoseResult(PosePollResult result)
{
    poseResult_ = std::move(result);
}

void MockVRProvider::pushEvent(const VREvent event)
{
    pendingEvents_.push_back(event);
}

} // namespace ovtr

