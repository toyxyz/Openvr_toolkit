#include "platform/win32/ViewportControlLayoutInternal.h"

#include "platform/win32/ViewportControlLayoutMetrics.h"

namespace ovtr::win32 {

void populateImportedAnimationControlLayout(ViewportControlLayout& layout) noexcept
{
    layout.animationBarRect = RECT{layout.barRect.left,
        layout.barRect.top - kImportedAnimationBarHeight, layout.barRect.right, layout.barRect.top};
    const int top = layout.animationBarRect.top + 4;
    const int center = (layout.animationBarRect.left + layout.animationBarRect.right) / 2;
    const int size = kImportedAnimationButtonSize;
    layout.firstFrameButtonRect = RECT{center - size - size / 2 - 2, top, center - size / 2 - 2, top + size};
    layout.playPauseButtonRect = RECT{center - size / 2, top, center + size / 2, top + size};
    layout.lastFrameButtonRect = RECT{center + size / 2 + 2, top, center + size + size / 2 + 2, top + size};
    layout.closeButtonRect = RECT{layout.animationBarRect.right - 80, top,
        layout.animationBarRect.right - 10, top + size};
    layout.frameTextRect = RECT{layout.animationBarRect.left + 12, top,
        layout.firstFrameButtonRect.left - 8, top + size};
    layout.timelineRect = RECT{layout.animationBarRect.left + 14, top + size + 5,
        layout.animationBarRect.right - 14, layout.animationBarRect.bottom - 5};
    layout.animationValid = layout.animationBarRect.top >= kViewportLayoutTopBarHeight &&
        layout.lastFrameButtonRect.right + 6 < layout.closeButtonRect.left &&
        layout.timelineRect.right - layout.timelineRect.left >= kImportedAnimationTimelineMinWidth;
}

} // namespace ovtr::win32
