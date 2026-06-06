#include "platform/win32/DeviceList.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

std::vector<DeviceListRow> makeDeviceListRows(const AppWindowState& state)
{
    return makeDeviceListRows(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state)
    );
}

std::vector<DeviceListRow> makeDevicePanelRows(const AppWindowState& state)
{
    return makeDevicePanelRows(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state)
    );
}

std::vector<DeviceListRow> makeSkeletalInputRows(const AppWindowState& state)
{
    return makeSkeletalInputRows(static_cast<const AppRuntimeState&>(state));
}

std::vector<DeviceListRow> makeSkeletalInputRows(const AppWindowState& state, const int sideIndex)
{
    return makeSkeletalInputRows(static_cast<const AppRuntimeState&>(state), sideIndex);
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
