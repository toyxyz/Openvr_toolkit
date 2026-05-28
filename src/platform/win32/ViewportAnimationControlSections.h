#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppImportedSceneState;

void drawImportedAnimationButtons(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    bool playing
);
void drawImportedAnimationTimeline(
    HDC drawDc,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
);
void drawImportedAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
);

} // namespace ovtr::win32
