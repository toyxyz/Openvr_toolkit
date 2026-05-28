#include "platform/win32/Layout.h"

#include "platform/win32/DeviceListLayoutMetrics.h"

namespace ovtr::win32 {

int maxDeviceListScrollOffset(const int totalItemCount, const int visibleItemCount) noexcept
{
    if (totalItemCount <= 0 || visibleItemCount <= 0) {
        return 0;
    }
    return totalItemCount > visibleItemCount ? totalItemCount - visibleItemCount : 0;
}

int clampDeviceListScrollOffset(
    const int scrollOffset,
    const int totalItemCount,
    const int visibleItemCount
) noexcept
{
    const int maxOffset = maxDeviceListScrollOffset(totalItemCount, visibleItemCount);
    if (scrollOffset < 0) {
        return 0;
    }
    if (scrollOffset > maxOffset) {
        return maxOffset;
    }
    return scrollOffset;
}

int deviceListItemTextRight(const DeviceListLayout& layout, const int totalItemCount) noexcept
{
    if (!layout.valid) {
        return 0;
    }
    return maxDeviceListScrollOffset(totalItemCount, layout.visibleItemCount) > 0
        ? layout.contentRect.right - 14
        : layout.contentRect.right;
}

int deviceListRowIndexFromPoint(
    const DeviceListLayout& layout,
    const POINT point,
    const int totalItemCount,
    const int scrollOffset
) noexcept
{
    if (!layout.valid || totalItemCount <= 0 || !PtInRect(&layout.contentRect, point)) {
        return -1;
    }

    if (point.x >= deviceListItemTextRight(layout, totalItemCount)) {
        return -1;
    }

    const int visibleRowIndex = (point.y - layout.contentRect.top) / kDeviceListItemHeight;
    if (visibleRowIndex < 0 || visibleRowIndex >= layout.visibleItemCount) {
        return -1;
    }

    const int rowIndex = scrollOffset + visibleRowIndex;
    return rowIndex >= 0 && rowIndex < totalItemCount ? rowIndex : -1;
}

} // namespace ovtr::win32
