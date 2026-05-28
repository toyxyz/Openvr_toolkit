#include "platform/win32/Layout.h"

namespace ovtr::win32 {
namespace {

constexpr int kDebugPanelLineHeight = 18;

} // namespace

int visibleDebugLineCountForRect(const RECT& bodyRect) noexcept
{
    const int height = bodyRect.bottom - bodyRect.top;
    if (height <= 0) {
        return 0;
    }
    return height / kDebugPanelLineHeight;
}

int maxDebugScrollOffset(const int totalLineCount, const int visibleLineCount) noexcept
{
    if (totalLineCount <= 0 || visibleLineCount <= 0) {
        return 0;
    }
    return totalLineCount > visibleLineCount ? totalLineCount - visibleLineCount : 0;
}

int clampDebugScrollOffset(
    const int scrollOffset,
    const int totalLineCount,
    const int visibleLineCount
) noexcept
{
    const int maxOffset = maxDebugScrollOffset(totalLineCount, visibleLineCount);
    if (scrollOffset < 0) {
        return 0;
    }
    if (scrollOffset > maxOffset) {
        return maxOffset;
    }
    return scrollOffset;
}

DebugScrollbarLayout debugScrollbarLayoutForRect(
    const RECT& bodyRect,
    const int totalLineCount,
    const int visibleLineCount,
    const int scrollOffset,
    const bool reverseScrollOrigin
) noexcept
{
    DebugScrollbarLayout layout;
    const int maxOffset = maxDebugScrollOffset(totalLineCount, visibleLineCount);
    const int trackHeight = bodyRect.bottom - bodyRect.top;
    if (maxOffset <= 0 || totalLineCount <= 0 || visibleLineCount <= 0 || trackHeight <= 0) {
        return layout;
    }

    layout.trackRect = RECT{bodyRect.right - 8, bodyRect.top, bodyRect.right, bodyRect.bottom};

    int thumbHeight = (trackHeight * visibleLineCount) / totalLineCount;
    if (thumbHeight < 18) {
        thumbHeight = 18;
    }
    if (thumbHeight > trackHeight) {
        thumbHeight = trackHeight;
    }

    const int clampedScrollOffset =
        clampDebugScrollOffset(scrollOffset, totalLineCount, visibleLineCount);
    const int thumbScrollOffset = reverseScrollOrigin
        ? maxOffset - clampedScrollOffset
        : clampedScrollOffset;
    const int travel = trackHeight - thumbHeight;
    const int thumbTop = layout.trackRect.top +
        (maxOffset > 0 ? (thumbScrollOffset * travel) / maxOffset : 0);
    layout.thumbRect = RECT{
        layout.trackRect.left,
        thumbTop,
        layout.trackRect.right,
        thumbTop + thumbHeight
    };
    layout.valid = true;
    return layout;
}

} // namespace ovtr::win32
