#include "platform/win32/SessionPanelPainter.h"

#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/DevicePanelScrollbarPainter.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

void paintSessionRow(
    HDC drawDc,
    const SessionListLayout& layout,
    const RecordingSessionListRow& row,
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
    DrawTextW(
        drawDc,
        row.name.c_str(),
        static_cast<int>(row.name.size()),
        &textRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
    );
}

} // namespace

void paintSessionListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppDebugUiState& state,
    const SessionListLayout& layout,
    const std::vector<RecordingSessionListRow>& rows
)
{
    if (!layout.valid) {
        return;
    }

    const int totalRows = static_cast<int>(rows.size());
    state.sessionListScrollOffset = clampSessionListScrollOffset(
        state.sessionListScrollOffset,
        totalRows,
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
    DrawTextW(drawDc, L"Sessions", -1, &headerText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(drawDc, bodyFont);
    const int textRight = sessionListItemTextRight(layout, totalRows);
    if (rows.empty()) {
        SetTextColor(drawDc, RGB(202, 211, 224));
        RECT emptyRect{layout.contentRect.left, layout.contentRect.top, textRight,
            layout.contentRect.top + kDeviceListItemHeight};
        DrawTextW(drawDc, L"No sessions", -1, &emptyRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    int rowTop = layout.contentRect.top;
    const int first = state.sessionListScrollOffset;
    const int last = first + layout.visibleItemCount < totalRows
        ? first + layout.visibleItemCount
        : totalRows;
    for (int i = first; i < last; ++i) {
        const RecordingSessionListRow& row = rows[static_cast<std::size_t>(i)];
        paintSessionRow(drawDc, layout, row, rowTop, textRight, row.name == state.selectedSessionName);
        rowTop += kDeviceListItemHeight;
    }

    const int maxScroll = maxSessionListScrollOffset(totalRows, layout.visibleItemCount);
    if (maxScroll > 0) {
        DeviceListLayout scrollbarLayout;
        scrollbarLayout.contentRect = layout.contentRect;
        scrollbarLayout.visibleItemCount = layout.visibleItemCount;
        scrollbarLayout.valid = true;
        paintDeviceListScrollbar(drawDc, scrollbarLayout, totalRows, maxScroll, state.sessionListScrollOffset);
    }
}

} // namespace ovtr::win32
