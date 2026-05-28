#include "platform/win32/DevicePanelRowPainter.h"

#include "platform/win32/Win32GdiResources.h"

#include <string>

namespace ovtr::win32 {

void paintEmptyDeviceListRow(
    HDC drawDc,
    const DeviceListLayout& layout,
    const DeviceListTableMetrics& metrics,
    const int itemY
)
{
    const std::wstring emptyText = L"No tracked devices";
    RECT itemRect{
        layout.contentRect.left,
        itemY,
        metrics.itemTextRight,
        itemY + kDeviceListItemHeight
    };
    DrawTextW(
        drawDc,
        emptyText.c_str(),
        static_cast<int>(emptyText.size()),
        &itemRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

void paintDeviceListRow(
    HDC drawDc,
    const DeviceListLayout& layout,
    const DeviceListTableMetrics& metrics,
    const DeviceListRow& row,
    const std::uint32_t selectedRuntimeIndex,
    const int itemY,
    const bool drawBottomGridLine
)
{
    const int rowBottom = itemY + kDeviceListItemHeight;
    const bool selected = row.runtimeIndex == selectedRuntimeIndex;
    if (selected) {
        UniqueBrush selectedRowBrush(CreateSolidBrush(RGB(38, 55, 78)));
        UniqueBrush selectedAccentBrush(CreateSolidBrush(RGB(98, 139, 190)));
        RECT highlightRect{layout.contentRect.left, itemY + 1, metrics.itemTextRight, rowBottom};
        FillRect(drawDc, &highlightRect, selectedRowBrush.get());

        RECT accentRect{layout.contentRect.left, itemY + 1, layout.contentRect.left + 3, rowBottom};
        FillRect(drawDc, &accentRect, selectedAccentBrush.get());
    }

    RECT customNameRect{
        layout.contentRect.left + (selected ? 8 : 0),
        itemY,
        metrics.nameColumnRight,
        rowBottom
    };
    RECT modelRect{metrics.modelColumnLeft, itemY, metrics.modelColumnRight, rowBottom};
    RECT serialRect{metrics.serialColumnLeft, itemY, metrics.itemTextRight, rowBottom};
    SetTextColor(drawDc, selected ? RGB(236, 242, 250) : RGB(202, 211, 224));
    DrawTextW(
        drawDc,
        row.customName.c_str(),
        static_cast<int>(row.customName.size()),
        &customNameRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
    DrawTextW(
        drawDc,
        row.model.c_str(),
        static_cast<int>(row.model.size()),
        &modelRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
    DrawTextW(
        drawDc,
        row.serial.c_str(),
        static_cast<int>(row.serial.size()),
        &serialRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );

    if (drawBottomGridLine) {
        UniquePen gridPen(CreatePen(PS_SOLID, 1, RGB(43, 48, 59)));
        SelectObjectGuard penSelection(drawDc, gridPen.get());
        MoveToEx(drawDc, layout.contentRect.left, rowBottom, nullptr);
        LineTo(drawDc, metrics.itemTextRight, rowBottom);
    }
}

} // namespace ovtr::win32
