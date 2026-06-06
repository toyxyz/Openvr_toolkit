#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"

#include <vector>

namespace ovtr::win32 {

std::uint32_t deviceRuntimeIndexFromListPoint(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const DeviceListLayout& layout,
    const POINT point
)
{
    const std::vector<DeviceListRow> rows = makeDevicePanelRows(runtimeState, deviceState);
    const int rowIndex = deviceListRowIndexFromPoint(
        layout,
        point,
        static_cast<int>(rows.size()),
        deviceState.deviceListScrollOffset
    );
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) {
        return kNoSelectedRuntimeIndex;
    }
    return rows[static_cast<std::size_t>(rowIndex)].runtimeIndex;
}

} // namespace ovtr::win32
