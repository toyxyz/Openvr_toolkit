#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/MappingPanelLayout.h"

namespace ovtr::win32 {

struct AppWindowState;

void drawMappingSoftIkFilter(HDC drawDc, HFONT font, const AppWindowState& state, const MappingPanelControlsLayout& controls);
void drawMappingSoftIkFilterDropdown(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const MappingPanelControlsLayout& controls
);
bool selectMappingSoftIkFilterDropdownOption(
    HWND hwnd,
    AppWindowState& state,
    const MappingPanelControlsLayout& controls,
    POINT point
);
bool toggleMappingSoftIkFilterDropdown(HWND hwnd, AppWindowState& state, const MappingPanelControlsLayout& controls, POINT point);

} // namespace ovtr::win32
