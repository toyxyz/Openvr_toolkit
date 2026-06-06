#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppOriginState;

void drawDeviceToggleButton(HDC drawDc, HFONT font, const RECT& rect, bool expanded);
void drawSessionToggleButton(HDC drawDc, HFONT font, const RECT& rect, bool expanded);
void drawProfileToggleButton(HDC drawDc, HFONT font, const RECT& rect, bool expanded);
void drawMappingToggleButton(HDC drawDc, HFONT font, const RECT& rect, bool expanded);
void drawEditToggleButton(HDC drawDc, HFONT font, const RECT& rect, bool expanded);
void drawTopBarMenuButton(HDC drawDc, HFONT font, const RECT& rect, const wchar_t* label, bool active);
void drawOriginStepperRow(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppOriginState& state,
    bool rotation
);

} // namespace ovtr::win32
