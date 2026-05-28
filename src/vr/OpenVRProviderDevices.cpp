#include "vr/OpenVRProvider.h"

#include "vr/OpenVRProviderDetails.h"
#include "vr/OpenVRProviderRuntime.h"

#ifdef OVTR_HAS_OPENVR_SDK
#include <openvr.h>
#endif

#include <utility>

namespace ovtr {

std::vector<DeviceDescriptor> OpenVRProvider::enumerateDevices() const
{
    std::vector<DeviceDescriptor> devices;
#ifdef OVTR_HAS_OPENVR_SDK
    if (!initialized_ || !runtime_) {
        return devices;
    }

    vr::IVRSystem* system = runtime_->system();
    if (system == nullptr) {
        return devices;
    }

    for (vr::TrackedDeviceIndex_t index = 0; index < vr::k_unMaxTrackedDeviceCount; ++index) {
        if (!system->IsTrackedDeviceConnected(index)) {
            continue;
        }

        const vr::ETrackedDeviceClass deviceClass = system->GetTrackedDeviceClass(index);
        if (deviceClass == vr::TrackedDeviceClass_Invalid) {
            continue;
        }

        DeviceDescriptor descriptor;
        descriptor.id = static_cast<DeviceId>(index + 1);
        descriptor.runtimeIndex = index;
        descriptor.deviceClass = openvr_provider_detail::mapDeviceClass(deviceClass);
        descriptor.serial = openvr_provider_detail::getStringProperty(system, index, vr::Prop_SerialNumber_String);
        descriptor.modelName = openvr_provider_detail::getStringProperty(system, index, vr::Prop_ModelNumber_String);
        descriptor.renderModelName =
            openvr_provider_detail::getStringProperty(system, index, vr::Prop_RenderModelName_String);
        descriptor.manufacturerName =
            openvr_provider_detail::getStringProperty(system, index, vr::Prop_ManufacturerName_String);
        descriptor.recordEnabled =
            descriptor.deviceClass == DeviceClass::Hmd ||
            descriptor.deviceClass == DeviceClass::Controller ||
            descriptor.deviceClass == DeviceClass::GenericTracker;
        devices.push_back(std::move(descriptor));
    }
#endif
    return devices;
}

} // namespace ovtr
