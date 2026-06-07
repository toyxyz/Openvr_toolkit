#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppWindowState;

void paintStreamingPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    const AppWindowState& state,
    const StreamingPanelLayout& layout
);

} // namespace ovtr::win32
