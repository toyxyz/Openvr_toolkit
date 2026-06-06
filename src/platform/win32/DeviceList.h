#pragma once

#include "data/SessionTypes.h"
#include "platform/win32/AppStateConstants.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppOriginState;
struct AppRuntimeState;
struct AppWindowState;

struct DeviceListRow {
    std::uint32_t runtimeIndex = kNoSelectedRuntimeIndex;
    std::wstring customName;
    std::wstring model;
    std::wstring serial;
};

std::string deviceNameKeyForParts(const std::string& deviceClass, const std::string& serial);
std::string deviceNameKeyForDevice(const ovtr::DeviceDescriptor& device);
bool splitDeviceNameKey(const std::string& key, std::string& deviceClass, std::string& serial);
std::string customNameForDevice(const AppDeviceState& state, const ovtr::DeviceDescriptor& device);
void applyCustomNamesToExportDevices(const AppDeviceState& state, std::vector<ovtr::DeviceDescriptor>& devices);
std::vector<DeviceListRow> makeDeviceListRows(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
);
std::vector<DeviceListRow> makeDeviceListRows(const AppWindowState& state);
std::vector<DeviceListRow> makeDevicePanelRows(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
);
std::vector<DeviceListRow> makeDevicePanelRows(const AppWindowState& state);
std::vector<DeviceListRow> makeSkeletalInputRows(const AppRuntimeState& runtimeState);
std::vector<DeviceListRow> makeSkeletalInputRows(const AppRuntimeState& runtimeState, int sideIndex);
std::vector<DeviceListRow> makeSkeletalInputRows(const AppWindowState& state);
std::vector<DeviceListRow> makeSkeletalInputRows(const AppWindowState& state, int sideIndex);
std::string deviceDisplayName(const ovtr::DeviceDescriptor& device);
std::string labelForDevice(
    const AppDeviceState& state,
    const ovtr::DeviceDescriptor* device,
    const ovtr::PoseSample& pose
);
ovtr::DeviceClass deviceClassForRuntimeIndex(
    const std::vector<ovtr::DeviceDescriptor>& devices,
    std::uint32_t runtimeIndex
);
const ovtr::DeviceDescriptor* deviceForRuntimeIndex(
    const std::vector<ovtr::DeviceDescriptor>& devices,
    std::uint32_t runtimeIndex
);
const ovtr::DeviceDescriptor* selectedOriginDevice(
    const AppRuntimeState& runtimeState,
    const AppOriginState& originState
);
const ovtr::DeviceDescriptor* selectedOriginDevice(const AppWindowState& state);
const ovtr::DeviceDescriptor* selectedListDevice(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
);
const ovtr::DeviceDescriptor* selectedListDevice(const AppWindowState& state);

} // namespace ovtr::win32
