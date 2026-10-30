#include "platform/win32/PaintWidgets.h"
#include "platform/win32/UiTheme.h"

#include <cwchar>

namespace ovtr::win32 {
namespace {

void drawRailTab(HDC dc, HFONT font, const RECT& rect, const wchar_t* label, const bool active)
{
    ui::fill(dc, rect, active ? ui::header : ui::window);
    if (active) {
        const RECT stripe{rect.left, rect.top + 3, rect.left + 2, rect.bottom - 3};
        ui::fill(dc, stripe, ui::accent);
    }
    // Rotate the complete word, preserving normal letter spacing and reading order.
    LOGFONTW description{};
    GetObjectW(font, sizeof(description), &description);
    description.lfEscapement = 900;
    description.lfOrientation = 900;
    UniqueFont tabFont(CreateFontIndirectW(&description));
    SelectObjectGuard fontSelection(dc, tabFont.get());
    const int length = static_cast<int>(std::wcslen(label));
    SIZE size{};
    GetTextExtentPoint32W(dc, label, length, &size);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, active ? ui::text : ui::muted);
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
    TextOutW(dc, rect.left + (rect.right - rect.left - size.cy) / 2,
        rect.top + (rect.bottom - rect.top + size.cx) / 2, label, length);
    if (saved) RestoreDC(dc, saved);
}

} // namespace

void drawDeviceToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Devices", active);
}

void drawSessionToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Sessions", active);
}

void drawStreamingToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Streaming", active);
}

void drawVideoToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Video", active);
}

void drawProfileToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Profile", active);
}

void drawMappingToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Mapping", active);
}

void drawEditToggleButton(HDC dc, HFONT font, const RECT& rect, const bool active)
{
    drawRailTab(dc, font, rect, L"Edit", active);
}

void drawTopBarMenuButton(HDC dc, HFONT font, const RECT& rect, const wchar_t* label, const bool active)
{
    if (rect.right <= rect.left || rect.bottom <= rect.top) return;
    if (active) ui::buttonSurface(dc, rect);
    SelectObjectGuard fontSelection(dc, font);
    SetTextColor(dc, ui::text);
    SetBkMode(dc, TRANSPARENT);
    RECT text = rect;
    DrawTextW(dc, label, -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

} // namespace ovtr::win32
