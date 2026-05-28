#pragma once

namespace ovtr::win32 {

inline constexpr int kDebugLayoutContentMargin = 32;
inline constexpr int kDebugLayoutStatusBarHeight = 36;
inline constexpr int kDebugMonitorMinHeight = 120;
inline constexpr int kDebugResizeGripHeight = 8;
inline constexpr int kDebugButtonWidth = 76;
inline constexpr int kDebugButtonHeight = 24;
inline constexpr int kDebugPanelPaddingTop = 10;

inline int debugLayoutStatusBarTopForClient(const int clientHeight) noexcept
{
    return clientHeight > kDebugLayoutStatusBarHeight
        ? clientHeight - kDebugLayoutStatusBarHeight
        : 0;
}

inline int debugLayoutMonitorTopForClient(
    const int activeDebugMonitorHeight,
    const int clientHeight
) noexcept
{
    const int statusBarTop = debugLayoutStatusBarTopForClient(clientHeight);
    if (activeDebugMonitorHeight <= 0) {
        return statusBarTop;
    }
    return statusBarTop > activeDebugMonitorHeight
        ? statusBarTop - activeDebugMonitorHeight
        : statusBarTop;
}

} // namespace ovtr::win32
