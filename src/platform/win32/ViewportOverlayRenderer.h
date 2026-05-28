#pragma once

namespace ovtr::win32 {

struct AppRecordingState;
struct AppViewportState;
struct AppWindowState;

void drawViewportOverlays2D(
    const AppRecordingState& recordingState,
    const AppViewportState& viewportState,
    int width,
    int height
);
void drawViewportOverlays2D(const AppWindowState& state, int width, int height);

} // namespace ovtr::win32
