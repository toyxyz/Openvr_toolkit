#include "TestCases.h"
#include "TestSupport.h"

#include "data/SkeletalSyntheticPose.h"
#include "data/VmcSyntheticPose.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"

namespace ovtr::test {
namespace {

ovtr::DeviceDescriptor makeDevice(
    const std::uint32_t runtimeIndex,
    const ovtr::DeviceClass deviceClass,
    const std::string& serial,
    const std::string& modelName
)
{
    ovtr::DeviceDescriptor device;
    device.runtimeIndex = runtimeIndex;
    device.deviceClass = deviceClass;
    device.serial = serial;
    device.modelName = modelName;
    return device;
}

} // namespace

void testWin32DeviceList()
{
    const std::string key = ovtr::win32::deviceNameKeyForParts("Controller", "ABC123");
    std::string deviceClass;
    std::string serial;
    require(ovtr::win32::splitDeviceNameKey(key, deviceClass, serial), "device name key splits");
    require(deviceClass == "Controller", "device name key preserves class");
    require(serial == "ABC123", "device name key preserves serial");
    require(!ovtr::win32::splitDeviceNameKey("Controller:ABC123", deviceClass, serial), "invalid key is rejected");

    ovtr::win32::AppWindowState state;
    state.devices.push_back(makeDevice(3, ovtr::DeviceClass::Controller, "CTRL", "Controller Model"));
    state.devices.push_back(makeDevice(1, ovtr::DeviceClass::Hmd, "HMD", "Headset"));
    state.devices.push_back(makeDevice(2, ovtr::DeviceClass::GenericTracker, "", "Tracker"));
    state.deviceCustomNames[ovtr::win32::deviceNameKeyForDevice(state.devices.front())] = "Left hand";

    const ovtr::win32::AppRuntimeState& runtimeState = state;
    const ovtr::win32::AppDeviceState& deviceState = state;
    const ovtr::win32::AppOriginState& originState = state;

    const std::vector<ovtr::win32::DeviceListRow> rows =
        ovtr::win32::makeDeviceListRows(runtimeState, deviceState);
    require(rows.size() == 3, "device list row count");
    require(rows[0].runtimeIndex == 1, "HMD rows sort before controllers");
    require(rows[1].runtimeIndex == 3, "controllers sort before trackers");
    require(rows[1].customName == L"Left hand", "custom device name appears in rows");
    require(rows[2].serial == L"(none)", "empty device serial uses placeholder");
    require(
        ovtr::win32::deviceClassForRuntimeIndex(state.devices, 1) == ovtr::DeviceClass::Hmd,
        "device class lookup"
    );
    require(
        ovtr::win32::deviceClassForRuntimeIndex(state.devices, 99) == ovtr::DeviceClass::Other,
        "missing device class lookup returns other"
    );
    require(
        ovtr::win32::deviceForRuntimeIndex(state.devices, 2) == &state.devices[2],
        "device pointer lookup"
    );
    require(
        ovtr::win32::deviceForRuntimeIndex(state.devices, 99) == nullptr,
        "missing device pointer lookup"
    );
    state.selectedOriginRuntimeIndex = 1;
    state.selectedDeviceRuntimeIndex = 3;
    require(
        ovtr::win32::selectedOriginDevice(runtimeState, originState) == &state.devices[1],
        "selected origin device lookup"
    );
    require(
        ovtr::win32::selectedListDevice(runtimeState, deviceState) == &state.devices[0],
        "selected list device lookup"
    );

    std::vector<ovtr::DeviceDescriptor> exportDevices = state.devices;
    ovtr::win32::applyCustomNamesToExportDevices(state, exportDevices);
    require(exportDevices[0].displayName == "Left hand", "custom device name is applied to export devices");
    require(ovtr::win32::deviceDisplayName(state.devices[0]) == "#3 Controller CTRL", "device display label");

    ovtr::PoseSample pose;
    pose.runtimeIndex = 42;
    require(
        ovtr::win32::labelForDevice(state, &state.devices[0], pose) == "Left hand",
        "device label prefers custom name"
    );
    require(
        ovtr::win32::labelForDevice(state, &state.devices[1], pose) == "HMD",
        "device label falls back to serial"
    );
    require(
        ovtr::win32::labelForDevice(state, nullptr, pose) == "#42",
        "device label falls back to runtime index"
    );

    ovtr::PoseSample leftFingerPose;
    leftFingerPose.runtimeIndex = ovtr::skeletalBoneRuntimeIndex(ovtr::SkeletalHandSide::Left, 7);
    ovtr::PoseSample rightFingerPose;
    rightFingerPose.runtimeIndex = ovtr::skeletalBoneRuntimeIndex(ovtr::SkeletalHandSide::Right, 12);
    state.poses.poses.push_back(leftFingerPose);
    state.poses.poses.push_back(rightFingerPose);
    ovtr::PoseSample leftVmcPose;
    leftVmcPose.runtimeIndex = ovtr::vmcFingerRuntimeIndex(ovtr::SkeletalHandSide::Left, 0);
    ovtr::PoseSample rightVmcPose;
    rightVmcPose.runtimeIndex = ovtr::vmcFingerRuntimeIndex(ovtr::SkeletalHandSide::Right, 0);
    state.poses.poses.push_back(leftVmcPose);
    state.poses.poses.push_back(rightVmcPose);

    const std::vector<ovtr::win32::DeviceListRow> panelRows =
        ovtr::win32::makeDevicePanelRows(state);
    require(panelRows.size() == 7, "device panel includes skeletal and VMC finger summary rows");
    require(panelRows[3].customName == L"Skeletal Input Left", "left skeletal input row label");
    require(panelRows[4].customName == L"Skeletal Input Right", "right skeletal input row label");
    require(panelRows[5].customName == L"VMC Finger Left", "left VMC finger row label");
    require(panelRows[6].customName == L"VMC Finger Right", "right VMC finger row label");
    require(
        ovtr::win32::makeFingerInputRows(state, 0).size() == 2,
        "left finger input rows include SteamVR and VMC"
    );
    require(
        ovtr::win32::makeFingerInputRows(state, 1).size() == 2,
        "right finger input rows include SteamVR and VMC"
    );
    require(
        ovtr::win32::makeDeviceListRows(state).size() == 3,
        "mapping device rows keep synthetic finger input hidden"
    );
}

} // namespace ovtr::test
