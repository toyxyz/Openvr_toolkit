#include "platform/win32/SessionPanelPainter.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/DevicePanelScrollbarPainter.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

std::wstring sessionRowMetadata(const RecordingSessionListRow& row)
{
    std::wstring text = row.createdLabel;
    if (row.frameCountKnown) {
        if (!text.empty()) {
            text += L" | ";
        }
        text += std::to_wstring(row.frameCount) + L" frames";
    }
    return text;
}

void paintSessionRow(
    HDC drawDc,
    const SessionListLayout& layout,
    const RecordingSessionListRow& row,
    const int rowTop,
    const int textRight,
    const bool selected
)
{
    const int rowBottom = rowTop + kSessionListItemHeight;
    if (selected) {
        UniqueBrush selectedBrush(CreateSolidBrush(ui::selection));
        UniqueBrush accentBrush(CreateSolidBrush(ui::accent));
        RECT highlight{layout.contentRect.left, rowTop + 1, textRight, rowBottom};
        RECT accent{layout.contentRect.left, rowTop + 1, layout.contentRect.left + 3, rowBottom};
        FillRect(drawDc, &highlight, selectedBrush.get());
        FillRect(drawDc, &accent, accentBrush.get());
    }

    UniquePen dividerPen(CreatePen(PS_SOLID, 1, ui::divider));
    {
        SelectObjectGuard penSelection(drawDc, dividerPen.get());
        MoveToEx(drawDc, layout.contentRect.left, rowBottom - 1, nullptr);
        LineTo(drawDc, textRight, rowBottom - 1);
    }

    const int textLeft = layout.contentRect.left + (selected ? 8 : 0);
    SetTextColor(drawDc, selected ? ui::onAccent : ui::text);
    RECT textRect{textLeft, rowTop + 3, textRight, rowTop + 22};
    DrawTextW(
        drawDc,
        row.displayName.c_str(),
        static_cast<int>(row.displayName.size()),
        &textRect,
        DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS
    );

    const std::wstring metadata = sessionRowMetadata(row);
    if (!metadata.empty()) {
        SetTextColor(drawDc, selected ? ui::onAccent : ui::muted);
        RECT metadataRect{textLeft, rowTop + 22, textRight, rowBottom - 3};
        DrawTextW(
            drawDc,
            metadata.c_str(),
            static_cast<int>(metadata.size()),
            &metadataRect,
            DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }
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

    UniqueBrush boxBrush(CreateSolidBrush(ui::panel));
    FillRect(drawDc, &layout.boxRect, boxBrush.get());
    UniquePen boxPen(CreatePen(PS_SOLID, 1, ui::border));
    {
        SelectObjectGuard penSelection(drawDc, boxPen.get());
        SelectObjectGuard brushSelection(drawDc, GetStockObject(NULL_BRUSH));
        Rectangle(drawDc, layout.boxRect.left, layout.boxRect.top, layout.boxRect.right, layout.boxRect.bottom);
    }

    UniqueBrush headerBrush(CreateSolidBrush(ui::header));
    FillRect(drawDc, &layout.headerRect, headerBrush.get());
    SelectObject(drawDc, headerFont ? headerFont : bodyFont);
    SetTextColor(drawDc, ui::muted);
    RECT headerText = layout.headerRect;
    DrawTextW(drawDc, L"Sessions", -1, &headerText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(drawDc, bodyFont);
    const int textRight = sessionListItemTextRight(layout, totalRows);
    if (rows.empty()) {
        SetTextColor(drawDc, ui::text);
        RECT emptyRect{layout.contentRect.left, layout.contentRect.top, textRight,
            layout.contentRect.top + kSessionListItemHeight};
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
        rowTop += kSessionListItemHeight;
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
