#include "platform/win32/ViewportAnimationControlPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/ViewportAnimationControlSections.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

void drawImportedAnimationControls(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppWindowState& state
)
{
    if (state.loadedSessionActive && layout.animationValid) {
        UniqueBrush barBrush(CreateSolidBrush(RGB(20, 23, 28)));
        FillRect(drawDc, &layout.animationBarRect, barBrush.get());
        UniquePen borderPen(CreatePen(PS_SOLID, 1, RGB(54, 58, 66)));
        {
            SelectObjectGuard penSelection(drawDc, borderPen.get());
            MoveToEx(drawDc, layout.animationBarRect.left, layout.animationBarRect.top, nullptr);
            LineTo(drawDc, layout.animationBarRect.right, layout.animationBarRect.top);
        }
        drawImportedAnimationButtons(drawDc, font, layout, state.loadedSessionPlaying);
        drawLoadedSessionAnimationTimeline(drawDc, layout, state);
        drawLoadedSessionAnimationFrameText(drawDc, font, layout, state);
        return;
    }
    drawImportedAnimationControls(
        drawDc,
        font,
        layout,
        static_cast<const AppImportedSceneState&>(state)
    );
}

} // namespace ovtr::win32
