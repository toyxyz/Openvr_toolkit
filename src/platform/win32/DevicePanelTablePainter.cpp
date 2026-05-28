#include "platform/win32/DevicePanelTablePainter.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/DevicePanelRowsPainter.h"
#include "platform/win32/DevicePanelScrollbarPainter.h"
#include "platform/win32/DevicePanelTableMetrics.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

void paintDeviceListTable(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppDeviceState& state,
    const DeviceListLayout& layout,
    const std::vector<DeviceListRow>& deviceRows
)
{
    const int totalDeviceRows = static_cast<int>(deviceRows.size());
    const int maxScrollOffset = maxDeviceListScrollOffset(totalDeviceRows, layout.visibleItemCount);
    const bool showScrollbar = maxScrollOffset > 0;
    const DeviceListTableMetrics metrics = deviceListTableMetricsForLayout(layout, totalDeviceRows);

    UniqueBrush headerBrush(CreateSolidBrush(RGB(24, 28, 35)));
    FillRect(drawDc, &layout.headerRect, headerBrush.get());

    UniquePen gridPenResource(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
    HPEN gridPen = gridPenResource.get();
    {
        SelectObjectGuard penSelection(drawDc, gridPen);
        MoveToEx(drawDc, metrics.nameDividerX, layout.headerRect.top, nullptr);
        LineTo(drawDc, metrics.nameDividerX, layout.contentRect.bottom);
        MoveToEx(drawDc, metrics.modelDividerX, layout.headerRect.top, nullptr);
        LineTo(drawDc, metrics.modelDividerX, layout.contentRect.bottom);
        MoveToEx(drawDc, layout.headerRect.left, layout.headerRect.bottom, nullptr);
        LineTo(drawDc, metrics.itemTextRight, layout.headerRect.bottom);
    }

    SelectObject(drawDc, headerFont ? headerFont : bodyFont);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT nameHeaderRect{metrics.nameColumnLeft, layout.headerRect.top, metrics.nameColumnRight, layout.headerRect.bottom};
    RECT modelHeaderRect{metrics.modelColumnLeft, layout.headerRect.top, metrics.modelColumnRight, layout.headerRect.bottom};
    RECT serialHeaderRect{metrics.serialColumnLeft, layout.headerRect.top, metrics.itemTextRight, layout.headerRect.bottom};
    DrawTextW(drawDc, L"Name", -1, &nameHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    DrawTextW(drawDc, L"Model", -1, &modelHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    DrawTextW(drawDc, L"Serial", -1, &serialHeaderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    paintDeviceListRows(drawDc, bodyFont, state, layout, metrics, deviceRows);
    if (!deviceRows.empty()) {
        if (showScrollbar) {
            paintDeviceListScrollbar(
                drawDc,
                layout,
                totalDeviceRows,
                maxScrollOffset,
                state.deviceListScrollOffset
            );
        }
    }
}

} // namespace ovtr::win32
