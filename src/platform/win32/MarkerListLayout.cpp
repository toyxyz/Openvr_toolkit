#include "platform/win32/Layout.h"

#include "platform/win32/DeviceListLayoutMetrics.h"

namespace ovtr::win32 {
namespace {

constexpr int kMarkerListBoxHeight = 180;
constexpr int kMarkerListMinimumBoxHeight = 104;

} // namespace

MarkerListLayout markerListLayoutForClient(
    const bool devicePanelVisible,
    const int leftPanelWidth,
    const int contentBottom,
    const int markerCount
) noexcept
{
    MarkerListLayout layout;
    if (!devicePanelVisible) {
        return layout;
    }

    const int boxLeft = kDeviceListToggleRailWidth + kDeviceListContentMargin - 8;
    const int boxRight = leftPanelWidth - 24;
    const int boxBottom = contentBottom > 12 ? contentBottom - 12 : contentBottom;
    const int topLimit = kDeviceListTopBarHeight + 12;
    const int availableHeight = boxBottom - topLimit;
    const int boxHeight = availableHeight < kMarkerListBoxHeight ? availableHeight : kMarkerListBoxHeight;
    const int boxTop = boxBottom - boxHeight;
    const int visibleBodyHeight = boxHeight - kDeviceListBoxPadding * 2 - kDeviceListHeaderHeight;
    const int visibleItemCount = visibleBodyHeight > 0
        ? visibleBodyHeight / kDeviceListItemHeight
        : 0;

    if (boxRight <= boxLeft + 80 ||
        boxHeight < kMarkerListMinimumBoxHeight ||
        visibleItemCount <= 0 ||
        markerCount < 0) {
        return layout;
    }

    layout.boxRect = RECT{boxLeft, boxTop, boxRight, boxBottom};
    layout.headerRect = RECT{
        boxLeft + kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding,
        boxRight - kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight
    };
    layout.contentRect = RECT{
        boxLeft + kDeviceListBoxPadding,
        layout.headerRect.bottom,
        boxRight - kDeviceListBoxPadding,
        boxBottom - kDeviceListBoxPadding
    };
    layout.visibleItemCount = visibleItemCount;
    layout.valid = layout.headerRect.right > layout.headerRect.left &&
        layout.contentRect.right > layout.contentRect.left &&
        layout.contentRect.bottom > layout.contentRect.top;
    return layout;
}

} // namespace ovtr::win32
