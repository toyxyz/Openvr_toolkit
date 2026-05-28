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
