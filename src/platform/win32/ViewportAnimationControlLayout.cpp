#include "platform/win32/ViewportControlLayoutInternal.h"

#include "platform/win32/ViewportControlLayoutMetrics.h"

namespace ovtr::win32 {

void populateImportedAnimationControlLayout(ViewportControlLayout& layout) noexcept
{
    layout.animationBarRect = RECT{
        layout.barRect.left,
        layout.barRect.top - kImportedAnimationBarHeight,
        layout.barRect.right,
        layout.barRect.top
    };

    const int buttonTop = layout.animationBarRect.top +
        (kImportedAnimationBarHeight - kImportedAnimationButtonSize) / 2;
    int x = layout.animationBarRect.left + 14;
    layout.firstFrameButtonRect = RECT{
        x,
        buttonTop,
        x + kImportedAnimationButtonSize,
        buttonTop + kImportedAnimationButtonSize
    };
    x = layout.firstFrameButtonRect.right + 8;
    layout.playPauseButtonRect = RECT{
        x,
        buttonTop,
        x + kImportedAnimationButtonSize,
        buttonTop + kImportedAnimationButtonSize
    };
    x = layout.playPauseButtonRect.right + 8;
    layout.lastFrameButtonRect = RECT{
        x,
        buttonTop,
        x + kImportedAnimationButtonSize,
        buttonTop + kImportedAnimationButtonSize
    };

    layout.closeButtonRect = RECT{
        layout.animationBarRect.right - 14 - kImportedAnimationCloseButtonWidth,
        buttonTop,
        layout.animationBarRect.right - 14,
        buttonTop + kImportedAnimationButtonSize
    };
    layout.frameTextRect = RECT{
        layout.closeButtonRect.left - 12 - kImportedAnimationFrameTextWidth,
        layout.animationBarRect.top,
        layout.closeButtonRect.left - 12,
        layout.animationBarRect.bottom
    };
    layout.timelineRect = RECT{
        layout.lastFrameButtonRect.right + 18,
        layout.animationBarRect.top + 14,
        layout.frameTextRect.left - 18,
        layout.animationBarRect.bottom - 14
    };

    layout.animationValid =
        layout.animationBarRect.bottom > layout.animationBarRect.top &&
        layout.closeButtonRect.right > layout.closeButtonRect.left &&
        layout.timelineRect.right - layout.timelineRect.left >= kImportedAnimationTimelineMinWidth;
}

} // namespace ovtr::win32
