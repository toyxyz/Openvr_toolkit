#include "vr/OpenVRProviderDetails.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <sstream>

namespace ovtr::openvr_provider_detail {

#ifdef OVTR_HAS_OPENVR_SDK
DeviceClass mapDeviceClass(const vr::ETrackedDeviceClass deviceClass)
{
    switch (deviceClass) {
    case vr::TrackedDeviceClass_HMD:
        return DeviceClass::Hmd;
    case vr::TrackedDeviceClass_Controller:
        return DeviceClass::Controller;
    case vr::TrackedDeviceClass_GenericTracker:
        return DeviceClass::GenericTracker;
    case vr::TrackedDeviceClass_TrackingReference:
        return DeviceClass::TrackingReference;
    case vr::TrackedDeviceClass_Invalid:
        return DeviceClass::Invalid;
    default:
        return DeviceClass::Other;
    }
}

VREventType mapEventType(const std::uint32_t eventType)
{
    switch (eventType) {
    case vr::VREvent_TrackedDeviceActivated:
        return VREventType::DeviceActivated;
    case vr::VREvent_TrackedDeviceDeactivated:
        return VREventType::DeviceDeactivated;
    case vr::VREvent_TrackedDeviceUpdated:
        return VREventType::DeviceUpdated;
    case vr::VREvent_Quit:
        return VREventType::Quit;
    default:
        return VREventType::Other;
    }
}

std::string getStringProperty(
    vr::IVRSystem* system,
    const vr::TrackedDeviceIndex_t index,
    const vr::ETrackedDeviceProperty property
)
{
    if (system == nullptr) {
        return {};
    }

    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    const std::uint32_t required = system->GetStringTrackedDeviceProperty(index, property, nullptr, 0, &error);
    if (required == 0 || (error != vr::TrackedProp_Success && error != vr::TrackedProp_BufferTooSmall)) {
        return {};
    }

    std::string value(required, '\0');
    error = vr::TrackedProp_Success;
    system->GetStringTrackedDeviceProperty(index, property, value.data(), required, &error);
    if (error != vr::TrackedProp_Success) {
        return {};
    }

    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

std::string initErrorMessage(const vr::EVRInitError error)
{
    std::ostringstream stream;
    stream << vr::VR_GetVRInitErrorAsSymbol(error) << ": "
           << vr::VR_GetVRInitErrorAsEnglishDescription(error);
    return stream.str();
}
#endif

} // namespace ovtr::openvr_provider_detail
