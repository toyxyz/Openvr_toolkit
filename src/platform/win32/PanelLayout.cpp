#include "platform/win32/Layout.h"

#include "platform/win32/PanelLayoutMetrics.h"

namespace ovtr::win32 {

int contentBottomForClient(const int activeDebugMonitorHeight, const int clientHeight) noexcept
{
    const int statusBarTop = panelStatusBarTopForClient(clientHeight);
    if (activeDebugMonitorHeight <= 0) {
        return statusBarTop;
    }
    return statusBarTop > activeDebugMonitorHeight
        ? statusBarTop - activeDebugMonitorHeight
        : statusBarTop;
}

RECT splitterRectForClient(
    const int leftPanelWidth,
    const int activeDebugMonitorHeight,
    const int clientHeight
) noexcept
{
    const int statusBarTop = panelStatusBarTopForClient(clientHeight);
    const int bottom = activeDebugMonitorHeight > 0 && statusBarTop > activeDebugMonitorHeight
        ? statusBarTop - activeDebugMonitorHeight
        : statusBarTop;
    return RECT{leftPanelWidth, kPanelTopBarHeight, leftPanelWidth + kPanelSplitterWidth, bottom};
}

RECT deviceToggleButtonRectForClient(
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    if (clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int top = kPanelTopBarHeight + 12;
    if (contentBottom <= top + 48) {
        return RECT{0, 0, 0, 0};
    }

    int height = kPanelDeviceToggleButtonHeight;
    const int availableHeight = contentBottom - top - 12;
    if (height > availableHeight) {
        height = availableHeight;
    }
    if (height < 48) {
        return RECT{0, 0, 0, 0};
    }

    const int left = (kPanelDeviceToggleRailWidth - kPanelDeviceToggleButtonWidth) / 2;
    return RECT{
        left,
        top,
        left + kPanelDeviceToggleButtonWidth,
        top + height
    };
}

} // namespace ovtr::win32
