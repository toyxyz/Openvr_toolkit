#include "platform/win32/DevicePanelRowsPainter.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/DevicePanelRowPainter.h"

namespace ovtr::win32 {

void paintDeviceListRows(
    HDC drawDc,
    HFONT bodyFont,
    const AppDeviceState& state,
    const DeviceListLayout& layout,
    const DeviceListTableMetrics& metrics,
    const std::vector<DeviceListRow>& deviceRows
)
{
    SelectObject(drawDc, bodyFont);
    SetTextColor(drawDc, RGB(202, 211, 224));
    int itemY = layout.contentRect.top;
    if (deviceRows.empty()) {
        paintEmptyDeviceListRow(drawDc, layout, metrics, itemY);
        return;
    }

    const int firstItemIndex = state.deviceListScrollOffset;
    const int lastItemIndex = firstItemIndex + layout.visibleItemCount < static_cast<int>(deviceRows.size())
        ? firstItemIndex + layout.visibleItemCount
        : static_cast<int>(deviceRows.size());

    for (int i = firstItemIndex; i < lastItemIndex; ++i) {
        const DeviceListRow& row = deviceRows[static_cast<std::size_t>(i)];
        const int rowBottom = itemY + kDeviceListItemHeight;
        const bool drawGridLine = i + 1 < lastItemIndex && rowBottom < layout.contentRect.bottom;
        paintDeviceListRow(
            drawDc,
            layout,
            metrics,
            row,
            state.selectedDeviceRuntimeIndex,
            itemY,
            drawGridLine
        );
        itemY += kDeviceListItemHeight;
    }
}

} // namespace ovtr::win32
