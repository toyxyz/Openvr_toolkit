#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;
struct ViewportControlLayout;

void importGlbFromFile(HWND hwnd, AppWindowState& state);
void closeImportedGlb(HWND hwnd, AppWindowState& state);
void invalidateImportedAnimationBar(HWND hwnd, const AppWindowState& state);
void seekImportedGlbFromTimeline(HWND hwnd, AppWindowState& state, const RECT& timelineRect, POINT point);
void toggleImportedGlbPlayback(HWND hwnd, AppWindowState& state);
void startImportedGlbPlaybackForRecording(HWND hwnd, AppWindowState& state);
bool handleImportedAnimationControlClick(
    HWND hwnd,
    AppWindowState& state,
    const ViewportControlLayout& layout,
    POINT point
);

} // namespace ovtr::win32
