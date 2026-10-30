#pragma once

#include "platform/win32/AppState.h"
#include "platform/win32/StatusBarPainter.h"
#include "platform/win32/ViewportControlPainter.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowPaintFonts.h"

namespace ovtr::win32 {

inline bool paintOnlyUpdatedControls(HDC dc, AppWindowState* state, const WindowPaintFonts& fonts,
    const int width, const int height, const int statusBarTop)
{
    RECT clip{};
    if (GetClipBox(dc, &clip) == ERROR || IsRectEmpty(&clip)) return false;
    if (clip.top >= statusBarTop) {
        paintStatusBar(dc, fonts.statusFont(), state, width, height, statusBarTop);
        return true;
    }
    if (!state) return false;
    const auto controls = viewportControlLayoutForClient(state, width, height);
    const int top = controls.importedSceneBarValid ? controls.importedSceneBarRect.top :
        (controls.animationValid ? controls.animationBarRect.top : controls.barRect.top);
    if (controls.valid && clip.top >= top && clip.bottom <= controls.barRect.bottom &&
        clip.left >= controls.barRect.left && clip.right <= controls.barRect.right) {
        drawViewportControlBar(dc, fonts.statusFont(), controls, *state);
        return true;
    }
    return false;
}

} // namespace ovtr::win32
