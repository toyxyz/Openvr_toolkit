#include "platform/win32/MarkerPanelPainter.h"

#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/DevicePanelScrollbarPainter.h"
#include "platform/win32/MarkerList.h"
#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32String.h"

namespace ovtr::win32 {
namespace {

void paintMarkerRow(
    HDC drawDc,
    const MarkerListLayout& layout,
    const SceneMarker& marker,
    const int rowTop,
    const int textRight,
    const bool selected
)
{
    const int rowBottom = rowTop + kDeviceListItemHeight;
    if (selected) {
        UniqueBrush selectedBrush(CreateSolidBrush(RGB(38, 55, 78)));
        UniqueBrush accentBrush(CreateSolidBrush(RGB(98, 139, 190)));
        RECT highlight{layout.contentRect.left, rowTop + 1, textRight, rowBottom};
        RECT accent{layout.contentRect.left, rowTop + 1, layout.contentRect.left + 3, rowBottom};
        FillRect(drawDc, &highlight, selectedBrush.get());
        FillRect(drawDc, &accent, accentBrush.get());
    }

    SetTextColor(drawDc, selected ? RGB(236, 242, 250) : RGB(202, 211, 224));
    RECT textRect{layout.contentRect.left + (selected ? 8 : 0), rowTop, textRight, rowBottom};
    const std::wstring name = widen(marker.name);
    DrawTextW(drawDc, name.c_str(), static_cast<int>(name.size()), &textRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

} // namespace

void paintMarkerListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppMarkerState& markerState,
    const MarkerListLayout& layout
)
{
    if (!layout.valid) {
        return;
    }

    markerState.markerListScrollOffset = clampMarkerListScrollOffset(
        markerState.markerListScrollOffset,
        static_cast<int>(markerState.markers.size()),
        layout.visibleItemCount
    );

    UniqueBrush boxBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &layout.boxRect, boxBrush.get());
    UniquePen boxPen(CreatePen(PS_SOLID, 1, RGB(58, 64, 76)));
    {
        SelectObjectGuard penSelection(drawDc, boxPen.get());
        SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(drawDc, layout.boxRect.left, layout.boxRect.top, layout.boxRect.right, layout.boxRect.bottom);
    }

    UniqueBrush headerBrush(CreateSolidBrush(RGB(24, 28, 35)));
    FillRect(drawDc, &layout.headerRect, headerBrush.get());
    SelectObject(drawDc, headerFont ? headerFont : bodyFont);
    SetTextColor(drawDc, RGB(168, 180, 196));
    RECT headerText = layout.headerRect;
    DrawTextW(drawDc, L"Markers", -1, &headerText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(drawDc, bodyFont);
    const int totalRows = static_cast<int>(markerState.markers.size());
    const int textRight = markerListItemTextRight(layout, totalRows);
    if (markerState.markers.empty()) {
        SetTextColor(drawDc, RGB(202, 211, 224));
        RECT emptyRect{layout.contentRect.left, layout.contentRect.top, textRight, layout.contentRect.top + kDeviceListItemHeight};
        DrawTextW(drawDc, L"No markers", -1, &emptyRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    int rowTop = layout.contentRect.top;
    const int first = markerState.markerListScrollOffset;
    const int last = first + layout.visibleItemCount < totalRows
        ? first + layout.visibleItemCount
        : totalRows;
    for (int i = first; i < last; ++i) {
        const SceneMarker& marker = markerState.markers[static_cast<std::size_t>(i)];
        paintMarkerRow(drawDc, layout, marker, rowTop, textRight, marker.id == markerState.selectedMarkerId);
        rowTop += kDeviceListItemHeight;
    }

    const int maxScroll = maxMarkerListScrollOffset(totalRows, layout.visibleItemCount);
    if (maxScroll > 0) {
        DeviceListLayout scrollbarLayout;
        scrollbarLayout.contentRect = layout.contentRect;
        scrollbarLayout.visibleItemCount = layout.visibleItemCount;
        scrollbarLayout.valid = true;
        paintDeviceListScrollbar(drawDc, scrollbarLayout, totalRows, maxScroll, markerState.markerListScrollOffset);
    }
}

} // namespace ovtr::win32
