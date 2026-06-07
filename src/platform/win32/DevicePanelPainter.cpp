#include "platform/win32/DevicePanelPainter.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/DevicePanelTablePainter.h"
#include "platform/win32/Layout.h"
#include "platform/win32/Win32GdiResources.h"

#include <vector>

namespace ovtr::win32 {

void paintDeviceListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppRuntimeState& runtimeState,
    AppDeviceState& deviceState,
    const DeviceListLayout& layout
)
{
    const std::vector<DeviceListRow> deviceRows = makeDevicePanelRows(runtimeState, deviceState);
    paintDeviceListPanelRows(drawDc, bodyFont, headerFont, deviceState, layout, deviceRows);
}

void paintDeviceListPanelRows(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppDeviceState& deviceState,
    const DeviceListLayout& layout,
    const std::vector<DeviceListRow>& deviceRows
)
{
    if (!layout.valid) {
        return;
    }

    deviceState.deviceListScrollOffset = clampDeviceListScrollOffset(
        deviceState.deviceListScrollOffset,
        static_cast<int>(deviceRows.size()),
        layout.visibleItemCount
    );

    RECT deviceBoxRect = layout.boxRect;
    UniqueBrush boxBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &deviceBoxRect, boxBrush.get());

    UniquePen boxPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    {
        SelectObjectGuard penSelection(drawDc, boxPen.get());
        SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(drawDc, deviceBoxRect.left, deviceBoxRect.top, deviceBoxRect.right, deviceBoxRect.bottom);
    }

    paintDeviceListTable(drawDc, bodyFont, headerFont, deviceState, layout, deviceRows);
}

} // namespace ovtr::win32
