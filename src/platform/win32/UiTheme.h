#pragma once

#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32::ui {

// Shared Blender-inspired editor palette. Viewport object colors remain configurable.
inline constexpr COLORREF window = RGB(40, 40, 40);
inline constexpr COLORREF panel = RGB(45, 45, 45);
inline constexpr COLORREF header = RGB(49, 49, 49);
inline constexpr COLORREF toolbar = RGB(48, 48, 48);
inline constexpr COLORREF field = RGB(35, 35, 35);
inline constexpr COLORREF button = RGB(77, 77, 77);
inline constexpr COLORREF buttonBorder = RGB(54, 54, 54);
inline constexpr COLORREF border = RGB(29, 29, 29);
inline constexpr COLORREF divider = RGB(57, 57, 57);
inline constexpr COLORREF text = RGB(222, 222, 222);
inline constexpr COLORREF muted = RGB(166, 166, 166);
inline constexpr COLORREF disabled = RGB(115, 115, 115);
inline constexpr COLORREF accent = RGB(71, 114, 179);
inline constexpr COLORREF selection = RGB(55, 83, 122);
inline constexpr COLORREF onAccent = RGB(245, 245, 245);
inline constexpr COLORREF scrollbar = RGB(91, 91, 91);
inline constexpr COLORREF record = RGB(218, 77, 83);
inline constexpr COLORREF recording = RGB(114, 47, 51);
inline constexpr COLORREF timeline = RGB(38, 38, 38);
inline constexpr COLORREF grid = RGB(48, 48, 48);
inline constexpr COLORREF timelineRuler = RGB(31, 31, 31);
inline constexpr COLORREF timelineTrack = RGB(47, 47, 47);
inline constexpr COLORREF timelineDivision = RGB(34, 34, 34);
inline constexpr wchar_t fontFamily[] = L"Tahoma";

inline void fill(HDC dc, const RECT& rect, const COLORREF color)
{
    UniqueBrush brush(CreateSolidBrush(color));
    FillRect(dc, &rect, brush.get());
}

inline void buttonSurface(HDC dc, const RECT& rect, const bool active = false)
{
    UniqueBrush brush(CreateSolidBrush(active ? accent : button));
    UniquePen pen(CreatePen(PS_SOLID, 1, active ? accent : buttonBorder));
    SelectObjectGuard brushSelection(dc, brush.get());
    SelectObjectGuard penSelection(dc, pen.get());
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 5, 5);
}

inline void chevron(HDC dc, const RECT& rect)
{
    const int x = (rect.left + rect.right) / 2;
    const int y = (rect.top + rect.bottom) / 2;
    UniquePen pen(CreatePen(PS_SOLID, 1, muted));
    SelectObjectGuard selection(dc, pen.get());
    MoveToEx(dc, x - 3, y - 2, nullptr);
    LineTo(dc, x, y + 1);
    LineTo(dc, x + 4, y - 3);
}

} // namespace ovtr::win32::ui
