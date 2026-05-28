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
        static_cast<const AppImportedSceneState&>(state)
    );
}

} // namespace ovtr::win32
