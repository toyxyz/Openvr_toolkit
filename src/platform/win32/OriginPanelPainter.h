#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppOriginState;
struct AppWindowState;

void paintOriginPanel(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppOriginState& state
);
void paintOriginPanel(
    HDC drawDc,
    HFONT labelFont,
    HFONT valueFont,
    const OriginPanelLayout& layout,
    const AppWindowState& state
);

} // namespace ovtr::win32
