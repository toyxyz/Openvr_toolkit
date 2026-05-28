#include "platform/win32/ViewportAnimationControlSections.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/Win32GdiResources.h"

#include <algorithm>

namespace ovtr::win32 {

void drawImportedAnimationTimeline(
    HDC drawDc,
    const ViewportControlLayout& layout,
    const AppImportedSceneState& state
)
{
    RECT trackRect{
        layout.timelineRect.left,
        layout.timelineRect.top + (layout.timelineRect.bottom - layout.timelineRect.top - 6) / 2,
        layout.timelineRect.right,
        layout.timelineRect.top + (layout.timelineRect.bottom - layout.timelineRect.top - 6) / 2 + 6
    };
    UniqueBrush trackBrush(CreateSolidBrush(RGB(48, 54, 66)));
    FillRect(drawDc, &trackRect, trackBrush.get());

    const double durationSeconds = importedSceneDurationSeconds(state);
    const double factor = durationSeconds > 0.0
        ? std::clamp(state.importedScenePlaybackSeconds / durationSeconds, 0.0, 1.0)
        : 0.0;
    RECT fillRect = trackRect;
    fillRect.right = fillRect.left + static_cast<int>(
        static_cast<double>(trackRect.right - trackRect.left) * factor
    );
    UniqueBrush fillBrush(CreateSolidBrush(RGB(212, 222, 236)));
    FillRect(drawDc, &fillRect, fillBrush.get());

    const int handleCenterX = fillRect.right;
    RECT handleRect{handleCenterX - 4, trackRect.top - 5, handleCenterX + 4, trackRect.bottom + 5};
    UniqueBrush handleBrush(CreateSolidBrush(RGB(255, 255, 255)));
    {
        SelectObjectGuard brushSelection(drawDc, handleBrush.get());
        SelectObjectGuard penSelection(drawDc, GetStockObject(NULL_PEN));
        Ellipse(drawDc, handleRect.left, handleRect.top, handleRect.right, handleRect.bottom);
    }
}

} // namespace ovtr::win32
