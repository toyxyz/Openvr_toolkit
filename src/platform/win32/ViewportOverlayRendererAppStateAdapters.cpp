#include "platform/win32/ViewportOverlayRenderer.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void drawViewportOverlays2D(const AppWindowState& state, const int width, const int height)
{
    drawViewportOverlays2D(
        static_cast<const AppRecordingState&>(state),
        static_cast<const AppViewportState&>(state),
        width,
        height
    );
}

} // namespace ovtr::win32
