#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/MappingPanelLayout.h"

namespace ovtr::win32 {

struct AppWindowState;

void drawMappingColorRow(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const MappingPanelControlsLayout& controls
);

} // namespace ovtr::win32
