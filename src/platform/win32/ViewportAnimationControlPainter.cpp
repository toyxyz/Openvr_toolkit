#include "platform/win32/ViewportAnimationControlPainter.h"
#include "platform/win32/UiTheme.h"

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
    if (!layout.animationValid) {
        return;
    }

    UniqueBrush barBrush(CreateSolidBrush(ui::panel));
    FillRect(drawDc, &layout.animationBarRect, barBrush.get());

    UniquePen borderPen(CreatePen(PS_SOLID, 1, ui::border));
    {
        SelectObjectGuard penSelection(drawDc, borderPen.get());
        MoveToEx(drawDc, layout.animationBarRect.left, layout.animationBarRect.top, nullptr);
        LineTo(drawDc, layout.animationBarRect.right, layout.animationBarRect.top);
    }

    drawImportedAnimationButtons(drawDc, font, layout, state.importedScenePlaying, state.importedSceneLoaded, false);
    if (state.importedSceneLoaded) {
        drawImportedAnimationTimeline(drawDc, layout, state);
        drawImportedAnimationFrameText(drawDc, font, layout, state);
    } else {
        SelectObjectGuard fontSelection(drawDc, font);
        SetTextColor(drawDc, ui::muted);
        RECT title = layout.frameTextRect;
        DrawTextW(drawDc, L"Timeline", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        drawEmptyAnimationTimeline(drawDc, layout);
    }
}

} // namespace ovtr::win32
