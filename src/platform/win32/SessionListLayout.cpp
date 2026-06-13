#include "platform/win32/Layout.h"

#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/RecordingSessionList.h"

namespace ovtr::win32 {
namespace {

constexpr int kSessionListBoxHeight = 900;
constexpr int kSessionListMinimumBoxHeight = 104;

} // namespace

SessionListLayout sessionListLayoutForClient(
    const bool sessionPanelVisible,
    const int leftPanelWidth,
    const int contentBottom,
    const bool lowerPanelValid,
    const int lowerPanelTop,
    const int sessionCount
) noexcept
{
    SessionListLayout layout;
    if (!sessionPanelVisible) {
        return layout;
    }

    const int boxLeft = kDeviceListToggleRailWidth + kDeviceListContentMargin - 8;
    const int boxRight = leftPanelWidth - 24;
    const int lowerLimit = lowerPanelValid
        ? lowerPanelTop - kDeviceListOriginPanelGap
        : (contentBottom > 12 ? contentBottom - 12 : contentBottom);
    const int topLimit = kDeviceListTopBarHeight + 12;
    const int availableHeight = lowerLimit - topLimit;
    const int boxHeight = availableHeight < kSessionListBoxHeight
        ? availableHeight
        : kSessionListBoxHeight;
    const int boxTop = topLimit;
    const int boxBottom = boxTop + boxHeight;
    const int visibleBodyHeight = boxHeight - kDeviceListBoxPadding * 2 - kDeviceListHeaderHeight;
    const int visibleItemCount = visibleBodyHeight > 0
        ? visibleBodyHeight / kSessionListItemHeight
        : 0;

    if (boxRight <= boxLeft + 80 ||
        boxHeight < kSessionListMinimumBoxHeight ||
        visibleItemCount <= 0 ||
        sessionCount < 0) {
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
