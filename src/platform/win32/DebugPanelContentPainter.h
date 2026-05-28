#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppDebugUiState;
struct AppImportedSceneState;
struct AppOriginState;
struct AppRecordingState;
struct AppRuntimeState;
struct AppViewportState;

void paintDebugMonitorContent(
    HDC drawDc,
    HFONT titleFont,
    HFONT bodyFont,
    const AppRuntimeState& runtimeState,
    const AppRecordingState& recordingState,
    const AppOriginState& originState,
    const AppImportedSceneState& importedSceneState,
    const AppViewportState& viewportState,
    AppDebugUiState& debugUiState,
    int clientWidth,
    int clientHeight,
    int activeDebugMonitorHeight,
    int debugMonitorTop
);

} // namespace ovtr::win32
