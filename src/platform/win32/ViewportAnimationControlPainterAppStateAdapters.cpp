#include "platform/win32/ViewportAnimationControlPainter.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void drawImportedAnimationControls(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
)
{
    drawImportedAnimationControls(
        drawDc,
        font,
        layout,
        static_cast<const AppImportedSceneState&>(state)
    );
}

} // namespace ovtr::win32
