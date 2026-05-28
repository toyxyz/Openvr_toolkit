#include "platform/win32/DeviceList.h"

#include "platform/win32/AppDeviceState.h"

namespace ovtr::win32 {

std::string deviceNameKeyForParts(const std::string& deviceClass, const std::string& serial)
{
    return deviceClass + '\x1f' + serial;
}

std::string deviceNameKeyForDevice(const ovtr::DeviceDescriptor& device)
{
    return deviceNameKeyForParts(ovtr::toString(device.deviceClass), device.serial);
}

bool splitDeviceNameKey(const std::string& key, std::string& deviceClass, std::string& serial)
{
    const std::size_t separator = key.find('\x1f');
    if (separator == std::string::npos) {
        return false;
    }

    deviceClass = key.substr(0, separator);
    serial = key.substr(separator + 1);
    return true;
}

std::string customNameForDevice(const AppDeviceState& state, const ovtr::DeviceDescriptor& device)
{
    const auto entry = state.deviceCustomNames.find(deviceNameKeyForDevice(device));
    if (entry == state.deviceCustomNames.end()) {
        return {};
    }
    return entry->second;
}

void applyCustomNamesToExportDevices(
    const AppDeviceState& state,
    std::vector<ovtr::DeviceDescriptor>& devices
)
{
    for (ovtr::DeviceDescriptor& device : devices) {
        device.displayName = customNameForDevice(state, device);
    }
}

} // namespace ovtr::win32
