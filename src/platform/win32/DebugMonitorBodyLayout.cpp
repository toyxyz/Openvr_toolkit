#include "platform/win32/Layout.h"

#include "platform/win32/DebugMonitorLayoutMetrics.h"

namespace ovtr::win32 {

RECT debugMessagesRectForClient(
    const int activeDebugMonitorHeight,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    const int statusBarTop = debugLayoutStatusBarTopForClient(clientHeight);
    if (activeDebugMonitorHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int debugMonitorTop = debugLayoutMonitorTopForClient(activeDebugMonitorHeight, clientHeight);
    const int splitX = clientWidth > 900 ? (clientWidth * 48) / 100 : clientWidth / 2;
    const int rightColumnLeft = splitX + 18;
    return RECT{
        rightColumnLeft,
        debugMonitorTop + kDebugPanelPaddingTop + 28,
        clientWidth - kDebugLayoutContentMargin,
        statusBarTop - 8
    };
}

RECT debugInfoRectForClient(
    const int activeDebugMonitorHeight,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    const int statusBarTop = debugLayoutStatusBarTopForClient(clientHeight);
    if (activeDebugMonitorHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int debugMonitorTop = debugLayoutMonitorTopForClient(activeDebugMonitorHeight, clientHeight);
    const int splitX = clientWidth > 900 ? (clientWidth * 48) / 100 : clientWidth / 2;
    const int leftColumnRight = splitX - 18;
    return RECT{
        kDebugLayoutContentMargin,
        debugMonitorTop + kDebugPanelPaddingTop + 28,
        leftColumnRight,
        statusBarTop - 8
    };
}

RECT debugResizeRectForClient(
    const int activeDebugMonitorHeight,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    const int statusBarTop = debugLayoutStatusBarTopForClient(clientHeight);
    if (activeDebugMonitorHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int debugMonitorTop = debugLayoutMonitorTopForClient(activeDebugMonitorHeight, clientHeight);
    const int bottom = debugMonitorTop + kDebugResizeGripHeight < statusBarTop
        ? debugMonitorTop + kDebugResizeGripHeight
        : statusBarTop;
    return RECT{0, debugMonitorTop, clientWidth, bottom};
}

} // namespace ovtr::win32
