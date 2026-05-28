#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppMarkerState.h"
#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

void paintMarkerListPanel(
    HDC drawDc,
    HFONT bodyFont,
    HFONT headerFont,
    AppMarkerState& markerState,
    const MarkerListLayout& layout
);

} // namespace ovtr::win32
