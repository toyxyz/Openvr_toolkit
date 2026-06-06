#include "platform/win32/MappingEditPresetPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/MappingOffsetPresetStore.h"
#include "platform/win32/Win32GdiResources.h"

#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

void drawButton(HDC dc, HFONT font, const RECT& rect, const wchar_t* text)
{
    UniqueBrush brush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 5, 5);
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(dc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawPresetBoxFrame(HDC dc, const RECT& rect)
{
    UniqueBrush brush(CreateSolidBrush(RGB(18, 22, 28)));
    FillRect(dc, &rect, brush.get());
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard penSelection(dc, pen.get());
    SelectObjectGuard brushSelection(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
}

} // namespace

void drawMappingEditPresetBox(
    HDC dc,
    HFONT font,
    const AppWindowState& state,
    const MappingEditPanelLayout& layout
)
{
    drawPresetBoxFrame(dc, layout.presetBoxRect);
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(168, 180, 196));
    RECT label = layout.presetNameLabelRect;
    DrawTextW(dc, L"Name", -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (!state.mappingEditOffsetPresetNameEditWindow) {
        UniqueBrush brush(CreateSolidBrush(RGB(30, 34, 42)));
        UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
        SelectObjectGuard brushSelection(dc, brush.get());
        SelectObjectGuard penSelection(dc, pen.get());
        RoundRect(
            dc,
            layout.presetNameEditRect.left,
            layout.presetNameEditRect.top,
            layout.presetNameEditRect.right,
            layout.presetNameEditRect.bottom,
            5,
            5
        );
        RECT textRect = layout.presetNameEditRect;
        textRect.left += 8;
        textRect.right -= 8;
        SetTextColor(dc, RGB(225, 231, 240));
        DrawTextW(
            dc,
            state.mappingEditOffsetPresetName.c_str(),
            -1,
            &textRect,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }

    drawButton(dc, font, layout.presetSaveButtonRect, L"Save");
    UniqueBrush brush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, layout.presetValueRect.left, layout.presetValueRect.top, layout.presetValueRect.right, layout.presetValueRect.bottom, 5, 5);
    RECT textRect = layout.presetValueRect;
    textRect.left += 8;
    textRect.right -= 24;
    SetTextColor(dc, RGB(225, 231, 240));
    DrawTextW(dc, L"Offset presets", -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    RECT arrowRect{layout.presetValueRect.right - 22, layout.presetValueRect.top, layout.presetValueRect.right - 6, layout.presetValueRect.bottom};
    DrawTextW(dc, L"v", -1, &arrowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawMappingEditPresetDropdown(HDC dc, HFONT font, const AppWindowState& state, const ProfilePanelLayout& panel)
{
    if (!state.mappingEditOffsetPresetDropdownOpen) {
        return;
    }
    std::vector<MappingOffsetPresetFileEntry> presets;
    std::string error;
    if (!listSavedMappingOffsetPresets(presets, error)) {
        presets.clear();
    }
    const int optionCount = static_cast<int>(presets.size());
    const RECT popup = mappingEditOffsetPresetDropdownRectForPanel(panel, optionCount);
    UniqueBrush brush(CreateSolidBrush(RGB(24, 28, 35)));
    UniquePen pen(CreatePen(PS_SOLID, 1, RGB(92, 126, 168)));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    Rectangle(dc, popup.left, popup.top, popup.right, popup.bottom);

    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    const int rowHeight = layout.presetValueRect.bottom - layout.presetValueRect.top;
    const int visibleRows = optionCount > 0 ? optionCount : 1;
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, RGB(225, 231, 240));
    for (int option = 0; option < visibleRows; ++option) {
        RECT row{popup.left + 6, popup.top + option * rowHeight, popup.right - 6, popup.top + (option + 1) * rowHeight};
        const std::wstring text = optionCount > 0 ? presets[static_cast<std::size_t>(option)].name : L"No presets";
        DrawTextW(dc, text.c_str(), -1, &row, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

} // namespace ovtr::win32
