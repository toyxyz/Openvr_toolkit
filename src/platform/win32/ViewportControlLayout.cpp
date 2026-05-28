#include "platform/win32/Layout.h"

#include "platform/win32/ViewportControlLayoutInternal.h"
#include "platform/win32/ViewportControlLayoutMetrics.h"

namespace ovtr::win32 {
namespace {

constexpr int kViewportControlBarSidePadding = 14;

} // namespace

ViewportControlLayout viewportControlLayoutForClient(
    const int leftPanelWidth,
    const int contentBottom,
    const bool showAnimationControls,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    ViewportControlLayout layout;
    if (clientWidth <= 0 || clientHeight <= 0) {
        return layout;
    }

    const int left = leftPanelWidth + kViewportLayoutSplitterWidth;
    const int right = clientWidth;
    const int availableWidth = right - left;
    const int availableHeight = contentBottom - kViewportLayoutTopBarHeight;
    const int controlHeight = kViewportControlBarHeight +
        (showAnimationControls ? kImportedAnimationBarHeight : 0);
    if (availableWidth < 240 || availableHeight < 180 + controlHeight) {
        return layout;
    }

    layout.barRect = RECT{
        left,
        contentBottom - kViewportControlBarHeight,
        right,
        contentBottom
    };

    const int buttonY = layout.barRect.top + (kViewportControlBarHeight - kViewportControlButtonSize) / 2;
    const int buttonTotalWidth = kViewportControlButtonSize;
    const int buttonLeft = left + (availableWidth - buttonTotalWidth) / 2;
    layout.quadViewButtonRect = RECT{
        left + kViewportControlBarSidePadding,
        buttonY,
        left + kViewportControlBarSidePadding + kViewportControlButtonSize,
        buttonY + kViewportControlButtonSize
    };
    layout.recordButtonRect = RECT{
        buttonLeft,
        buttonY,
        buttonLeft + kViewportControlButtonSize,
        buttonY + kViewportControlButtonSize
    };

    layout.valid = layout.recordButtonRect.right > layout.recordButtonRect.left &&
        layout.barRect.bottom > layout.barRect.top;

    if (showAnimationControls) {
        populateImportedAnimationControlLayout(layout);
    }
    return layout;
}

RECT viewportRenderRectForClient(
    const int leftPanelWidth,
    const int contentBottom,
    const ViewportControlLayout& controls,
    const int clientWidth
) noexcept
{
    const int left = leftPanelWidth + kViewportLayoutSplitterWidth;
    const int bottom = controls.animationValid
        ? controls.animationBarRect.top
        : (controls.valid ? controls.barRect.top : contentBottom);
    return RECT{left, kViewportLayoutTopBarHeight, clientWidth, bottom};
}

} // namespace ovtr::win32
