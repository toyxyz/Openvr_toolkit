#include "platform/win32/Layout.h"

#include "platform/win32/DeviceListLayoutMetrics.h"

namespace ovtr::win32 {

DeviceListLayout deviceListLayoutForClient(
    const bool devicePanelVisible,
    const int leftPanelWidth,
    const int contentBottom,
    const bool originPanelValid,
    const int originPanelTop,
    const int deviceCount
) noexcept
{
    DeviceListLayout layout;
    if (!devicePanelVisible) {
        return layout;
    }

    const int textRight = leftPanelWidth - 24;
    const int listBottom = originPanelValid
        ? originPanelTop - kDeviceListOriginPanelGap
        : (contentBottom > 12 ? contentBottom - 12 : contentBottom);
    const int textBottom = listBottom > 0 ? listBottom : 0;

    const int boxLeft = kDeviceListToggleRailWidth + kDeviceListContentMargin - 8;
    const int boxRight = textRight;
    const int sessionTop = kDeviceListTopBarHeight + 12;
    const int sessionBottom = sessionTop + kDeviceListSessionBoxHeight;
    const int boxTop = sessionBottom + kDeviceListSessionBoxGap;
    const int visibleBodyHeight = textBottom - boxTop - kDeviceListBoxPadding * 2 -
        kDeviceListHeaderHeight;
    const int visibleItemCount = visibleBodyHeight > 0
        ? visibleBodyHeight / kDeviceListItemHeight
        : 0;
    const int rowsForBox = deviceCount <= 0 ? 1 : deviceCount;
    const int drawnRows = rowsForBox < visibleItemCount ? rowsForBox : visibleItemCount;
    const int boxBottom = boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight +
        drawnRows * kDeviceListItemHeight + kDeviceListBoxPadding;

    if (boxRight <= boxLeft + 80 ||
        drawnRows <= 0 ||
        boxBottom <= boxTop + kDeviceListBoxPadding ||
        boxTop >= textBottom) {
        return layout;
    }

    layout.boxRect = RECT{
        boxLeft,
        boxTop,
        boxRight,
        boxBottom < textBottom ? boxBottom : textBottom
    };
    layout.sessionBoxRect = RECT{boxLeft, sessionTop, boxRight, sessionBottom};
    layout.sessionLabelRect = RECT{boxLeft + 12, sessionTop, boxLeft + 92, sessionBottom};
    layout.sessionValueRect = RECT{boxLeft + 96, sessionTop + 6, boxRight - 12, sessionBottom - 6};
    layout.headerRect = RECT{
        boxLeft + kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding,
        boxRight - kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight
    };
    layout.contentRect = RECT{
        boxLeft + kDeviceListBoxPadding,
        boxTop + kDeviceListBoxPadding + kDeviceListHeaderHeight,
        boxRight - kDeviceListBoxPadding,
        layout.boxRect.bottom - kDeviceListBoxPadding
    };
    layout.visibleItemCount = visibleItemCount;
    layout.valid = layout.headerRect.right > layout.headerRect.left &&
        layout.headerRect.bottom > layout.headerRect.top &&
        layout.contentRect.right > layout.contentRect.left &&
        layout.contentRect.bottom > layout.contentRect.top;
    return layout;
}

} // namespace ovtr::win32
