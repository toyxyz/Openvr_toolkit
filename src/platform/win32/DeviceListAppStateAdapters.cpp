#include "platform/win32/DeviceList.h"

#include "platform/win32/AppState.h"

#include <algorithm>

namespace ovtr::win32 {
namespace {

void appendMissingRows(std::vector<DeviceListRow>& rows, const std::vector<DeviceListRow>& additions)
{
    for (const DeviceListRow& row : additions) {
        const auto found = std::find_if(
            rows.begin(),
            rows.end(),
            [&row](const DeviceListRow& existing) {
                return existing.runtimeIndex == row.runtimeIndex;
            }
        );
        if (found == rows.end()) {
            rows.push_back(row);
        }
    }
}

} // namespace

std::vector<DeviceListRow> makeDeviceListRows(const AppWindowState& state)
{
    return makeDeviceListRows(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state)
    );
}

std::vector<DeviceListRow> makeDevicePanelRows(const AppWindowState& state)
{
    std::vector<DeviceListRow> rows = makeDevicePanelRows(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state)
    );
    if (state.vmcReceiveEnabled) {
        appendMissingRows(rows, makeVmcFingerInputRows(state.vmcReceiver.snapshot()));
    }
    return rows;
}

std::vector<DeviceListRow> makeSkeletalInputRows(const AppWindowState& state)
{
    return makeSkeletalInputRows(static_cast<const AppRuntimeState&>(state));
}

std::vector<DeviceListRow> makeSkeletalInputRows(const AppWindowState& state, const int sideIndex)
{
    return makeSkeletalInputRows(static_cast<const AppRuntimeState&>(state), sideIndex);
}

std::vector<DeviceListRow> makeFingerInputRows(const AppWindowState& state, const int sideIndex)
{
    std::vector<DeviceListRow> rows = makeFingerInputRows(static_cast<const AppRuntimeState&>(state), sideIndex);
    if (state.vmcReceiveEnabled) {
        appendMissingRows(rows, makeVmcFingerInputRows(state.vmcReceiver.snapshot(), sideIndex));
    }
    return rows;
}

const ovtr::DeviceDescriptor* selectedOriginDevice(const AppWindowState& state)
{
    return selectedOriginDevice(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppOriginState&>(state)
    );
}

const ovtr::DeviceDescriptor* selectedListDevice(const AppWindowState& state)
{
    return selectedListDevice(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state)
    );
}

} // namespace ovtr::win32
