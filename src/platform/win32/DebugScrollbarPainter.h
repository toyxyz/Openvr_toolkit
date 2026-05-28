#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

void paintDebugScrollbar(
    HDC drawDc,
    const RECT& bodyRect,
    int totalLineCount,
    int visibleLineCount,
    int scrollOffset,
    bool reverseScrollOrigin
);

} // namespace ovtr::win32
