#include "platform/win32/DeviceList.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <sstream>

namespace ovtr::win32 {
namespace {

int deviceListRank(const ovtr::DeviceDescriptor& device)
{
    const std::string modelName = lowerAscii(device.modelName);
    const bool isValveTrackingReference =
        modelName.find("valve sr imp") != std::string::npos ||
        modelName.find("valve sr") != std::string::npos;

    if (device.deviceClass == ovtr::DeviceClass::Hmd) {
        return 0;
    }
    if (device.deviceClass == ovtr::DeviceClass::TrackingReference || isValveTrackingReference) {
        return 1;
    }
    if (device.deviceClass == ovtr::DeviceClass::Controller) {
        return 2;
    }
    if (device.deviceClass == ovtr::DeviceClass::GenericTracker) {
        return 3;
    }
    return 4;
}

} // namespace

std::vector<DeviceListRow> makeDeviceListRows(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
)
{
    std::vector<const ovtr::DeviceDescriptor*> sortedDevices;
    sortedDevices.reserve(runtimeState.devices.size());
    for (const ovtr::DeviceDescriptor& device : runtimeState.devices) {
        sortedDevices.push_back(&device);
    }

    std::stable_sort(
        sortedDevices.begin(),
        sortedDevices.end(),
        [](const ovtr::DeviceDescriptor* left, const ovtr::DeviceDescriptor* right) {
            return deviceListRank(*left) < deviceListRank(*right);
        }
    );

    std::vector<DeviceListRow> rows;
    rows.reserve(sortedDevices.size());
    for (const ovtr::DeviceDescriptor* device : sortedDevices) {
        rows.push_back(DeviceListRow{
            device->runtimeIndex,
            widen(customNameForDevice(deviceState, *device)),
            widen(device->modelName.empty() ? ovtr::toString(device->deviceClass) : device->modelName),
            widen(device->serial.empty() ? "(none)" : device->serial),
        });
    }
    return rows;
}

std::string deviceDisplayName(const ovtr::DeviceDescriptor& device)
{
    std::ostringstream stream;
    stream << "#" << device.runtimeIndex << " " << ovtr::toString(device.deviceClass);
    if (!device.serial.empty()) {
        stream << " " << device.serial;
    }
    return stream.str();
}

std::string labelForDevice(
    const AppDeviceState& state,
    const ovtr::DeviceDescriptor* device,
    const ovtr::PoseSample& pose
)
{
    if (device) {
        const std::string customName = customNameForDevice(state, *device);
        if (!customName.empty()) {
            return customName;
        }
    }

    if (device && !device->serial.empty()) {
        return device->serial;
    }

    std::ostringstream stream;
    stream << "#" << pose.runtimeIndex;
    return stream.str();
}

} // namespace ovtr::win32
