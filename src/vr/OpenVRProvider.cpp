#include "vr/OpenVRProvider.h"

#include "vr/OpenVRProviderDetails.h"
#include "vr/OpenVRProviderRuntime.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <memory>

namespace ovtr {

void OpenVRProviderRuntimeDeleter::operator()(OpenVRProviderRuntime* runtime) const noexcept
{
    delete runtime;
}

OpenVRProvider::~OpenVRProvider()
{
    shutdown();
}

bool OpenVRProvider::initialize()
{
#ifdef OVTR_HAS_OPENVR_SDK
    lastError_.clear();
    if (initialized_) {
        return true;
    }

    if (!vr::VR_IsRuntimeInstalled()) {
        state_ = VRConnectionState::RuntimeNotInstalled;
        lastError_ = "OpenVR runtime is not installed";
        return false;
    }

    if (!vr::VR_IsHmdPresent()) {
        state_ = VRConnectionState::HmdNotPresent;
    }

    vr::EVRInitError error = vr::VRInitError_None;
    std::unique_ptr<OpenVRProviderRuntime, OpenVRProviderRuntimeDeleter> runtime(new OpenVRProviderRuntime(
        vr::VR_Init(&error, vr::VRApplication_Background)
    ));
    if (error != vr::VRInitError_None || !*runtime) {
        initialized_ = false;
        state_ = VRConnectionState::InitFailed;
        lastError_ = openvr_provider_detail::initErrorMessage(error);
        return false;
    }

    runtime_ = std::move(runtime);
    initialized_ = true;
    state_ = VRConnectionState::Connected;
    return true;
#else
    initialized_ = false;
    state_ = VRConnectionState::InitFailed;
    lastError_ = "OpenVR SDK integration has not been enabled yet";
    return false;
#endif
}

void OpenVRProvider::shutdown()
{
    runtime_.reset();
    initialized_ = false;
    state_ = VRConnectionState::SteamVRNotRunning;
}

bool OpenVRProvider::isInitialized() const
{
    return initialized_;
}

VRConnectionState OpenVRProvider::connectionState() const
{
    return state_;
}

std::string OpenVRProvider::lastError() const
{
    return lastError_;
}

} // namespace ovtr
