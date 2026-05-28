#include "platform/win32/ViewportAnimationControlPainter.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/ViewportAnimationControlSections.h"
#include "platform/win32/Win32GdiResources.h"

namespace ovtr::win32 {

void drawImportedAnimationControls(
    HDC drawDc,
    HFONT font,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
)
{
    if (!layout.animationValid || !state.importedSceneLoaded) {
        return;
    }

    UniqueBrush barBrush(CreateSolidBrush(RGB(20, 23, 28)));
    FillRect(drawDc, &layout.animationBarRect, barBrush.get());

    UniquePen borderPen(CreatePen(PS_SOLID, 1, RGB(54, 58, 66)));
    {
        SelectObjectGuard penSelection(drawDc, borderPen.get());
        MoveToEx(drawDc, layout.animationBarRect.left, layout.animationBarRect.top, nullptr);
        LineTo(drawDc, layout.animationBarRect.right, layout.animationBarRect.top);
    }

    drawImportedAnimationButtons(drawDc, font, layout, state.importedScenePlaying);
    drawImportedAnimationTimeline(drawDc, layout, state);
    drawImportedAnimationFrameText(drawDc, font, layout, state);
}

} // namespace ovtr::win32
