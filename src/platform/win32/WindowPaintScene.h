#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/WindowPaintFonts.h"

namespace ovtr::win32 {

struct AppWindowState;

void paintWindowScene(
    HDC drawDc,
    AppWindowState* state,
    const WindowPaintFonts& fonts,
    int clientWidth,
    int clientHeight
);

} // namespace ovtr::win32
