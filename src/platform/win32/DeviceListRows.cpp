#include "platform/win32/DeviceList.h"

#include "data/SkeletalSyntheticPose.h"
#include "data/VmcSyntheticPose.h"
#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/VmcFingerState.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <utility>

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

void appendSkeletalInputRow(
    std::vector<DeviceListRow>& rows,
    const ovtr::SkeletalHandSide side
)
{
    const bool isLeft = side == ovtr::SkeletalHandSide::Left;
    rows.push_back(DeviceListRow{
        ovtr::skeletalBoneRuntimeIndex(side, 1),
        isLeft ? L"Skeletal Input Left" : L"Skeletal Input Right",
        L"SteamVR Skeletal Input",
        isLeft ? L"Left hand" : L"Right hand",
    });
}

void appendVmcFingerInputRow(
    std::vector<DeviceListRow>& rows,
    const ovtr::SkeletalHandSide side
)
{
    const bool isLeft = side == ovtr::SkeletalHandSide::Left;
    rows.push_back(DeviceListRow{
        ovtr::vmcFingerRuntimeIndex(side, 0),
        isLeft ? L"VMC Finger Left" : L"VMC Finger Right",
        L"VMC Finger Input",
        isLeft ? L"VMC-FINGER-L" : L"VMC-FINGER-R",
    });
}

std::vector<DeviceListRow> skeletalInputRowsForPoses(const ovtr::PosePollResult& poses)
{
    std::vector<DeviceListRow> rows;
    bool hasLeft = false;
    bool hasRight = false;
    for (const ovtr::PoseSample& pose : poses.poses) {
        ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
        std::uint32_t boneIndex = 0;
        if (!ovtr::decodeSkeletalBoneRuntimeIndex(pose.runtimeIndex, side, boneIndex)) {
            continue;
        }
        hasLeft = hasLeft || side == ovtr::SkeletalHandSide::Left;
        hasRight = hasRight || side == ovtr::SkeletalHandSide::Right;
    }
    if (hasLeft) {
        appendSkeletalInputRow(rows, ovtr::SkeletalHandSide::Left);
    }
    if (hasRight) {
        appendSkeletalInputRow(rows, ovtr::SkeletalHandSide::Right);
    }
    return rows;
}

std::vector<DeviceListRow> vmcFingerRowsForPoses(const ovtr::PosePollResult& poses)
{
    std::vector<DeviceListRow> rows;
    bool hasLeft = false;
    bool hasRight = false;
    for (const ovtr::PoseSample& pose : poses.poses) {
        ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
        std::uint32_t boneIndex = 0;
        if (!ovtr::decodeVmcFingerRuntimeIndex(pose.runtimeIndex, side, boneIndex)) {
            continue;
        }
        hasLeft = hasLeft || side == ovtr::SkeletalHandSide::Left;
        hasRight = hasRight || side == ovtr::SkeletalHandSide::Right;
    }
    if (hasLeft) {
        appendVmcFingerInputRow(rows, ovtr::SkeletalHandSide::Left);
    }
    if (hasRight) {
        appendVmcFingerInputRow(rows, ovtr::SkeletalHandSide::Right);
    }
    return rows;
}

std::vector<DeviceListRow> filterFingerRowsBySide(std::vector<DeviceListRow> rows, const int sideIndex)
{
    if (sideIndex < 0 || sideIndex > 1) {
        return rows;
    }
    const ovtr::SkeletalHandSide expectedSide =
        sideIndex == 0 ? ovtr::SkeletalHandSide::Left : ovtr::SkeletalHandSide::Right;
    rows.erase(
        std::remove_if(
            rows.begin(),
            rows.end(),
            [expectedSide](const DeviceListRow& row) {
                ovtr::SkeletalHandSide side = ovtr::SkeletalHandSide::Left;
                std::uint32_t boneIndex = 0;
                const bool skeletal = ovtr::decodeSkeletalBoneRuntimeIndex(row.runtimeIndex, side, boneIndex);
                const bool vmc = !skeletal && ovtr::decodeVmcFingerRuntimeIndex(row.runtimeIndex, side, boneIndex);
                return (!skeletal && !vmc) || side != expectedSide;
            }
        ),
        rows.end()
    );
    return rows;
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

std::vector<DeviceListRow> makeDevicePanelRows(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
)
{
    std::vector<DeviceListRow> rows = makeDeviceListRows(runtimeState, deviceState);
    const std::vector<DeviceListRow> skeletalRows = makeSkeletalInputRows(runtimeState);
    rows.insert(rows.end(), skeletalRows.begin(), skeletalRows.end());
    const std::vector<DeviceListRow> vmcRows = vmcFingerRowsForPoses(runtimeState.poses);
    rows.insert(rows.end(), vmcRows.begin(), vmcRows.end());
    return rows;
}

std::vector<DeviceListRow> makeSkeletalInputRows(const AppRuntimeState& runtimeState)
{
    return skeletalInputRowsForPoses(runtimeState.poses);
}

std::vector<DeviceListRow> makeSkeletalInputRows(const AppRuntimeState& runtimeState, const int sideIndex)
{
    return filterFingerRowsBySide(makeSkeletalInputRows(runtimeState), sideIndex);
}

std::vector<DeviceListRow> makeVmcFingerInputRows(const VmcFingerSnapshot& snapshot)
{
    std::vector<DeviceListRow> rows;
    const auto now = std::chrono::steady_clock::now();
    for (const ovtr::SkeletalHandSide side : {ovtr::SkeletalHandSide::Left, ovtr::SkeletalHandSide::Right}) {
        const VmcFingerSideState& hand = snapshot.hands[static_cast<std::size_t>(vmcSideIndex(side))];
        if (isVmcFingerSideFresh(hand, now)) {
            appendVmcFingerInputRow(rows, side);
        }
    }
    return rows;
}

std::vector<DeviceListRow> makeVmcFingerInputRows(const VmcFingerSnapshot& snapshot, const int sideIndex)
{
    return filterFingerRowsBySide(makeVmcFingerInputRows(snapshot), sideIndex);
}

std::vector<DeviceListRow> makeFingerInputRows(const AppRuntimeState& runtimeState, const int sideIndex)
{
    std::vector<DeviceListRow> rows = makeSkeletalInputRows(runtimeState, sideIndex);
    std::vector<DeviceListRow> vmcRows = vmcFingerRowsForPoses(runtimeState.poses);
    vmcRows = filterFingerRowsBySide(std::move(vmcRows), sideIndex);
    rows.insert(rows.end(), vmcRows.begin(), vmcRows.end());
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
