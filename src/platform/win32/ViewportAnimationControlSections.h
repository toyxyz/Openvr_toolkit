#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppImportedSceneState;
struct AppLoadedSessionState;

void drawImportedAnimationButtons(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    bool playing,
    bool enabled = true,
    bool showClose = true
);
void drawEmptyAnimationTimeline(HDC drawDc, const ViewportControlLayout& layout);
void drawCombinedAnimationTimeline(HDC dc, const ViewportControlLayout& layout,
    const AppImportedSceneState& glb, const AppLoadedSessionState& session);
void drawCombinedAnimationFrameText(HDC dc, HFONT font, const ViewportControlLayout& layout,
    const AppImportedSceneState& glb, const AppLoadedSessionState& session);
void drawImportedAnimationTimeline(
    HDC drawDc,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
);
void drawLoadedSessionAnimationTimeline(
    HDC drawDc,
    const ViewportControlLayout& layout,
    const AppLoadedSessionState& state
);
void drawImportedAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
);
void drawLoadedSessionAnimationFrameText(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppLoadedSessionState& state
);

} // namespace ovtr::win32
