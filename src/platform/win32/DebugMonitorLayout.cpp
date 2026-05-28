#include "platform/win32/Layout.h"

#include "platform/win32/DebugMonitorLayoutMetrics.h"

namespace ovtr::win32 {

int maxDebugMonitorHeightForClient(const int clientHeight) noexcept
{
    const int maximumHeight =
        clientHeight - kDebugLayoutStatusBarHeight - kDebugLayoutContentMargin - 180;
    return maximumHeight > 0 ? maximumHeight : 0;
}

int clampDebugMonitorHeightForClient(const int requestedHeight, const int clientHeight) noexcept
{
    const int maximumHeight = maxDebugMonitorHeightForClient(clientHeight);
    if (maximumHeight <= 0) {
        return 0;
    }

    int minimumHeight = kDebugMonitorMinHeight;
    if (minimumHeight > maximumHeight) {
        minimumHeight = maximumHeight;
    }

    if (requestedHeight < minimumHeight) {
        return minimumHeight;
    }
    if (requestedHeight > maximumHeight) {
        return maximumHeight;
    }
    return requestedHeight;
}

RECT debugButtonRectForClient(const int clientWidth, const int clientHeight) noexcept
{
    const int statusBarTop = debugLayoutStatusBarTopForClient(clientHeight);
    const int right = clientWidth > kDebugLayoutContentMargin
        ? clientWidth - kDebugLayoutContentMargin
        : clientWidth;
    const int left = right > kDebugButtonWidth ? right - kDebugButtonWidth : 0;
    const int top = statusBarTop + (kDebugLayoutStatusBarHeight - kDebugButtonHeight) / 2;
    return RECT{left, top, right, top + kDebugButtonHeight};
}

} // namespace ovtr::win32
