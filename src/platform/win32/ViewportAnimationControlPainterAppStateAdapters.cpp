#include "platform/win32/ViewportAnimationControlPainter.h"
#include "platform/win32/ViewportImportedSceneBar.h"
#include "platform/win32/UiTheme.h"

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
    drawImportedSceneInfoBar(drawDc, font, layout, state);
    if (state.loadedSessionActive && layout.animationValid) {
        UniqueBrush barBrush(CreateSolidBrush(ui::panel));
        FillRect(drawDc, &layout.animationBarRect, barBrush.get());
        UniquePen borderPen(CreatePen(PS_SOLID, 1, ui::border));
        {
            SelectObjectGuard penSelection(drawDc, borderPen.get());
            MoveToEx(drawDc, layout.animationBarRect.left, layout.animationBarRect.top, nullptr);
            LineTo(drawDc, layout.animationBarRect.right, layout.animationBarRect.top);
        }
        drawImportedAnimationButtons(drawDc, font, layout, state.loadedSessionPlaying);
        drawCombinedAnimationTimeline(drawDc, layout, state, state);
        drawCombinedAnimationFrameText(drawDc, font, layout, state, state);
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
