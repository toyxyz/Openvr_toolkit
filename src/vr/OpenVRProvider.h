#pragma once

#include "vr/IVRProvider.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

namespace ovtr {

class OpenVRProvider final : public IVRProvider {
public:
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;
    VRConnectionState connectionState() const override;
    std::string lastError() const override;

    bool pollEvents(std::vector<VREvent>& outEvents) override;
    bool pollPoses(PosePollResult& outResult) override;
    std::vector<DeviceDescriptor> enumerateDevices() const override;

private:
#ifdef OVTR_HAS_OPENVR_SDK
    vr::IVRSystem* system_ = nullptr;
#endif
    bool initialized_ = false;
    VRConnectionState state_ = VRConnectionState::RuntimeNotInstalled;
    std::string lastError_ = "OpenVR provider is not linked in this build yet";
};

} // namespace ovtr
