#include "platform/win32/ViewportAnimationControlSections.h"

#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {
namespace {

void drawImportedAnimationButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label)
{
    UniqueBrush buttonBrush(CreateSolidBrush(RGB(30, 34, 42)));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, RGB(67, 74, 88)));
    {
        SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
        SelectObjectGuard penSelection(drawDc, buttonPen.get());
        RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    }

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

} // namespace

void drawImportedAnimationButtons(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const bool playing
)
{
    drawImportedAnimationButton(drawDc, font, layout.firstFrameButtonRect, L"|<");
    drawImportedAnimationButton(drawDc, font, layout.playPauseButtonRect, playing ? L"||" : L">");
    drawImportedAnimationButton(drawDc, font, layout.lastFrameButtonRect, L">|");
    drawImportedAnimationButton(drawDc, font, layout.closeButtonRect, L"Close");
}

} // namespace ovtr::win32
