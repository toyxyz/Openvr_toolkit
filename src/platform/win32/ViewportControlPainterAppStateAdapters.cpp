#include "platform/win32/ViewportControlPainter.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void drawViewportControlBar(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
)
{
    drawViewportControlBar(
        drawDc,
        font,
        layout,
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppImportedSceneState&>(state),
        static_cast<const AppLoadedSessionState&>(state),
        static_cast<const AppSessionState&>(state),
        static_cast<const AppStreamingState&>(state),
        static_cast<const AppViewportState&>(state),
        state.trackedDevicesVisible
    );
}

} // namespace ovtr::win32
