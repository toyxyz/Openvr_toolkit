#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppWindowState;

void paintVideoPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppWindowState& state,
    const VideoPanelLayout& layout
);

} // namespace ovtr::win32
