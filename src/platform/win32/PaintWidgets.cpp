#include "platform/win32/PaintWidgets.h"

#include "platform/win32/AppState.h"
#include "platform/win32/Win32GdiResources.h"

#include <cwchar>

namespace ovtr::win32 {
namespace {

COLORREF buttonFillColor(const bool active) noexcept
{
    return active ? RGB(48, 63, 82) : RGB(30, 34, 42);
}

COLORREF buttonBorderColor(const bool active) noexcept
{
    return active ? RGB(92, 126, 168) : RGB(67, 74, 88);
}

void drawVerticalToggleButton(
    HDC drawDc,
    HFONT font,
    const RECT& rect,
    const wchar_t* label,
    const bool expanded,
    const int preferredHeight
)
{
    UniqueBrush buttonBrush(CreateSolidBrush(buttonFillColor(expanded)));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, buttonBorderColor(expanded)));
    {
        SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
        SelectObjectGuard penSelection(drawDc, buttonPen.get());
        RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    }

    SelectObjectGuard fontSelection(drawDc, font);

    SetTextColor(drawDc, RGB(225, 231, 240));
    const int labelLength = static_cast<int>(std::wcslen(label));
    const int totalHeight = rect.bottom - rect.top;
    const int lineHeight = totalHeight >= preferredHeight ? 14 : (totalHeight / (labelLength + 1));
    const int labelHeight = lineHeight * labelLength;
    int y = rect.top + (totalHeight - labelHeight) / 2;
    for (int i = 0; i < labelLength; ++i) {
        RECT charRect{rect.left, y, rect.right, y + lineHeight};
        DrawTextW(drawDc, label + i, 1, &charRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        y += lineHeight;
    }
}

} // namespace

void drawDeviceToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Device", expanded, 84);
}

void drawProfileToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Profile", expanded, 112);
}

void drawMappingToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Mapping", expanded, 112);
}

void drawTopBarMenuButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label, const bool active)
{
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }

    UniqueBrush buttonBrush(CreateSolidBrush(buttonFillColor(active)));
    UniquePen buttonPen(CreatePen(PS_SOLID, 1, buttonBorderColor(active)));
    {
        SelectObjectGuard brushSelection(drawDc, buttonBrush.get());
        SelectObjectGuard penSelection(drawDc, buttonPen.get());
        RoundRect(drawDc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    }

    if (font) {
        SelectObject(drawDc, font);
    }
    SetTextColor(drawDc, RGB(225, 231, 240));
    RECT textRect = rect;
    DrawTextW(drawDc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

} // namespace ovtr::win32
