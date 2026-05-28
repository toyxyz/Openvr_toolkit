#pragma once

#include "vr/IVRProvider.h"

#include <cstdint>
#include <memory>
#include <string>

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
    struct SkeletalInputState {
        bool setupAttempted = false;
        bool available = false;
        std::uint64_t actionSet = 0;
        std::uint64_t leftAction = 0;
        std::uint64_t rightAction = 0;
        std::uint32_t leftBoneCount = 0;
        std::uint32_t rightBoneCount = 0;
        std::string error;
    };

    bool initializeSkeletalInput();
    void appendSkeletalPoses(PosePollResult& outResult);

    std::unique_ptr<OpenVRProviderRuntime, OpenVRProviderRuntimeDeleter> runtime_;
    SkeletalInputState skeletalInput_;
    bool initialized_ = false;
    VRConnectionState state_ = VRConnectionState::RuntimeNotInstalled;
    std::string lastError_ = "OpenVR provider is not linked in this build yet";
};

} // namespace ovtr
