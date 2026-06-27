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

int verticalLabelLineHeight(HDC drawDc, const int totalHeight, const int labelLength, const int preferredHeight) noexcept
{
    if (labelLength <= 0 || totalHeight <= 0) {
        return 0;
    }

    TEXTMETRICW metrics{};
    const int naturalLineHeight = GetTextMetricsW(drawDc, &metrics) ? metrics.tmHeight + 1 : 14;
    const int paddedHeight = totalHeight > 8 ? totalHeight - 8 : totalHeight;
    const int availableLineHeight = paddedHeight / labelLength;
    int lineHeight = naturalLineHeight;
    if (availableLineHeight > 0 && availableLineHeight < lineHeight) {
        lineHeight = availableLineHeight;
    }
    if (lineHeight < 1) {
        return 1;
    }
    return lineHeight;
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
    SetBkMode(drawDc, TRANSPARENT);
    const int labelLength = static_cast<int>(std::wcslen(label));
    const int totalHeight = rect.bottom - rect.top;
    const int lineHeight = verticalLabelLineHeight(drawDc, totalHeight, labelLength, preferredHeight);
    const int labelHeight = lineHeight * labelLength;
    int y = rect.top + (totalHeight - labelHeight) / 2;
    const int savedDc = SaveDC(drawDc);
    if (savedDc) {
        IntersectClipRect(drawDc, rect.left, rect.top, rect.right, rect.bottom);
    }
    for (int i = 0; i < labelLength; ++i) {
        RECT charRect{rect.left, y, rect.right, y + lineHeight};
        DrawTextW(drawDc, label + i, 1, &charRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        y += lineHeight;
    }
    if (savedDc) {
        RestoreDC(drawDc, savedDc);
    }
}

} // namespace

void drawDeviceToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Device", expanded, 84);
}

void drawSessionToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Session", expanded, 112);
}

void drawStreamingToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Stream", expanded, 128);
}

void drawProfileToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Profile", expanded, 112);
}

void drawMappingToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Mapping", expanded, 112);
}

void drawEditToggleButton(HDC drawDc, HFONT font, const RECT& rect, const bool expanded)
{
    drawVerticalToggleButton(drawDc, font, rect, L"Edit", expanded, 64);
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
