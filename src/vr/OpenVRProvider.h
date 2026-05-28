#pragma once

#include "vr/IVRProvider.h"

#include <memory>

namespace ovtr {

class OpenVRProviderRuntime;

struct OpenVRProviderRuntimeDeleter {
    void operator()(OpenVRProviderRuntime* runtime) const noexcept;
};

class OpenVRProvider final : public IVRProvider {
public:
    ~OpenVRProvider() override;

    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;
    VRConnectionState connectionState() const override;
    std::string lastError() const override;

    bool pollEvents(std::vector<VREvent>& outEvents) override;
    bool pollPoses(PosePollResult& outResult) override;
    std::vector<DeviceDescriptor> enumerateDevices() const override;

private:
    std::unique_ptr<OpenVRProviderRuntime, OpenVRProviderRuntimeDeleter> runtime_;
    bool initialized_ = false;
    VRConnectionState state_ = VRConnectionState::RuntimeNotInstalled;
    std::string lastError_ = "OpenVR provider is not linked in this build yet";
};

} // namespace ovtr
