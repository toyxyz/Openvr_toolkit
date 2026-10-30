#include "platform/win32/MappingPanelColorPainter.h"
#include "platform/win32/UiTheme.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

void drawBox(HDC dc, const RECT& rect)
{
    UniqueBrush brush(CreateSolidBrush(ui::field));
    UniquePen pen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
}

void drawButton(HDC dc, HFONT font, const RECT& rect)
{
    UniqueBrush brush(CreateSolidBrush(ui::button));
    UniquePen pen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, ui::text);
    RECT textRect = rect;
    DrawTextW(dc, L"Pick", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

RgbColor previewColorForState(const AppWindowState& state) noexcept
{
    const MappingActor* actor = selectedMappingActor(state);
    return actor != nullptr ? actor->skeletonColor : state.mappingSkeletonColor;
}

} // namespace

void drawMappingColorRow(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const MappingPanelControlsLayout& controls
)
{
    drawBox(drawDc, controls.colorBoxRect);
    UniquePen divider(CreatePen(PS_SOLID, 1, ui::divider));
    SelectObjectGuard penSelection(drawDc, divider.get());
    MoveToEx(drawDc, controls.colorLabelRect.right + 8, controls.colorBoxRect.top, nullptr);
    LineTo(drawDc, controls.colorLabelRect.right + 8, controls.colorBoxRect.bottom);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, ui::muted);
    RECT labelRect = controls.colorLabelRect;
    DrawTextW(drawDc, L"Color", -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    const RgbColor color = clampRgbColor(previewColorForState(state));
    UniqueBrush swatch(CreateSolidBrush(RGB(color.r, color.g, color.b)));
    FillRect(drawDc, &controls.colorSwatchRect, swatch.get());
    UniquePen swatchPen(CreatePen(PS_SOLID, 1, ui::buttonBorder));
    SelectObjectGuard swatchPenSelection(drawDc, swatchPen.get());
    MoveToEx(drawDc, controls.colorSwatchRect.left, controls.colorSwatchRect.top, nullptr);
    LineTo(drawDc, controls.colorSwatchRect.right - 1, controls.colorSwatchRect.top);
    LineTo(drawDc, controls.colorSwatchRect.right - 1, controls.colorSwatchRect.bottom - 1);
    LineTo(drawDc, controls.colorSwatchRect.left, controls.colorSwatchRect.bottom - 1);
    LineTo(drawDc, controls.colorSwatchRect.left, controls.colorSwatchRect.top);
    drawButton(drawDc, font, controls.colorPickButtonRect);
}

} // namespace ovtr::win32
