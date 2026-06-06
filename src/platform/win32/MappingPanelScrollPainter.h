#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;
struct MappingPanelControlsLayout;

void drawScrollbar(HDC drawDc, const RECT& tableRect, int visibleRows, int scrollOffset);
void drawActorScrollbar(HDC drawDc, const MappingPanelControlsLayout& controls, const AppWindowState& state);

} // namespace ovtr::win32
