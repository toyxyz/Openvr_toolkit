#include "platform/win32/DevicePanelPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"

#include <vector>

namespace ovtr::win32 {

void paintDeviceListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppWindowState& state,
    const DeviceListLayout& layout
)
{
    const std::vector<DeviceListRow> deviceRows = makeDevicePanelRows(state);
    paintDeviceListPanelRows(
        drawDc,
        bodyFont,
        headerFont,
        static_cast<AppDeviceState&>(state),
        layout,
        deviceRows
    );
}

} // namespace ovtr::win32
