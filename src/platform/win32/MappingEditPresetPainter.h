#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"
#include "platform/win32/MappingEditPanelLayout.h"

namespace ovtr::win32 {

struct AppWindowState;

void drawMappingEditPresetBox(HDC dc, HFONT font, const AppWindowState& state, const MappingEditPanelLayout& layout);
void drawMappingEditPresetDropdown(HDC dc, HFONT font, const AppWindowState& state, const ProfilePanelLayout& panel);

} // namespace ovtr::win32
