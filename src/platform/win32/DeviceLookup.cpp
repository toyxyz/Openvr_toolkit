#include "platform/win32/DeviceList.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppOriginState.h"
#include "platform/win32/AppRuntimeState.h"

namespace ovtr::win32 {

ovtr::DeviceClass deviceClassForRuntimeIndex(
    const std::vector<ovtr::DeviceDescriptor>& devices,
    const std::uint32_t runtimeIndex
)
{
    for (const ovtr::DeviceDescriptor& device : devices) {
        if (device.runtimeIndex == runtimeIndex) {
            return device.deviceClass;
        }
    }
    return ovtr::DeviceClass::Other;
}

const ovtr::DeviceDescriptor* deviceForRuntimeIndex(
    const std::vector<ovtr::DeviceDescriptor>& devices,
    const std::uint32_t runtimeIndex
)
{
    for (const ovtr::DeviceDescriptor& device : devices) {
        if (device.runtimeIndex == runtimeIndex) {
            return &device;
        }
    }
    return nullptr;
}

const ovtr::DeviceDescriptor* selectedOriginDevice(
    const AppRuntimeState& runtimeState,
    const AppOriginState& originState
)
{
    return deviceForRuntimeIndex(runtimeState.devices, originState.selectedOriginRuntimeIndex);
}

const ovtr::DeviceDescriptor* selectedListDevice(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
)
{
    return deviceForRuntimeIndex(runtimeState.devices, deviceState.selectedDeviceRuntimeIndex);
}

} // namespace ovtr::win32
