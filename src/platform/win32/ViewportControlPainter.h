#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

namespace ovtr::win32 {

struct AppImportedSceneState;
struct AppRecordingState;
struct AppWindowState;

void drawViewportControlBar(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppRecordingState& recordingState,
    const AppImportedSceneState& importedSceneState
);
void drawViewportControlBar(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
);

} // namespace ovtr::win32
