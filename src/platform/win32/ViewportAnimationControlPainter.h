#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppImportedSceneState;
struct AppWindowState;

void drawImportedAnimationControls(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
);
void drawImportedAnimationControls(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
);

} // namespace ovtr::win32
