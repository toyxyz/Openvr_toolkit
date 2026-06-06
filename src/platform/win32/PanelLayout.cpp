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

RECT sessionToggleButtonRectForClient(
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    const RECT deviceRect = deviceToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (deviceRect.right <= deviceRect.left || deviceRect.bottom <= deviceRect.top) {
        return RECT{0, 0, 0, 0};
    }

    const int top = deviceRect.bottom + 8;
    if (contentBottom <= top + 48) {
        return RECT{0, 0, 0, 0};
    }

    int height = kPanelSessionToggleButtonHeight;
    const int availableHeight = contentBottom - top - 12;
    if (height > availableHeight) {
        height = availableHeight;
    }
    if (height < 48) {
        return RECT{0, 0, 0, 0};
    }

    return RECT{deviceRect.left, top, deviceRect.right, top + height};
}

int rightProfileAreaWidthForClient(const bool profilePanelVisible, const int clientWidth) noexcept
{
    return rightProfileAreaWidthForClient(
        profilePanelVisible,
        kPanelProfilePanelDefaultWidth,
        clientWidth
    );
}

int defaultProfilePanelWidthForClient(const int clientWidth) noexcept
{
    return clampProfilePanelWidthForClient(kPanelProfilePanelDefaultWidth, clientWidth);
}

int clampProfilePanelWidthForClient(const int requestedWidth, const int clientWidth) noexcept
{
    const int maxByViewport = clientWidth - kPanelProfileToggleRailWidth -
        kPanelProfileSplitterWidth - kPanelContentMargin - kViewportMinWidth;
    if (maxByViewport <= 0) {
        return clientWidth > kPanelProfileToggleRailWidth
            ? clientWidth - kPanelProfileToggleRailWidth
            : 0;
    }

    int maxWidth = maxByViewport < kPanelProfilePanelMaxWidth
        ? maxByViewport
        : kPanelProfilePanelMaxWidth;
    if (maxWidth < kPanelProfilePanelMinWidth) {
        maxWidth = maxByViewport;
    }

    int minWidth = kPanelProfilePanelMinWidth;
    if (minWidth > maxWidth) {
        minWidth = maxWidth;
    }

    if (requestedWidth < minWidth) {
        return minWidth;
    }
    if (requestedWidth > maxWidth) {
        return maxWidth;
    }
    return requestedWidth;
}

int rightProfileAreaWidthForClient(
    const bool profilePanelVisible,
    const int requestedWidth,
    const int clientWidth
) noexcept
{
    if (clientWidth <= 0) {
        return 0;
    }

    int width = kPanelProfileToggleRailWidth;
    if (profilePanelVisible) {
        width += kPanelProfileSplitterWidth +
            clampProfilePanelWidthForClient(requestedWidth, clientWidth);
    }
    return width < clientWidth ? width : clientWidth;
}

RECT profileToggleButtonRectForClient(
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

    int height = kPanelProfileToggleButtonHeight;
    const int availableHeight = contentBottom - top - 12;
    if (height > availableHeight) {
        height = availableHeight;
    }
    if (height < 48) {
        return RECT{0, 0, 0, 0};
    }

    const int left = clientWidth - kPanelProfileToggleRailWidth +
        (kPanelProfileToggleRailWidth - kPanelProfileToggleButtonWidth) / 2;
    return RECT{
        left,
        top,
        left + kPanelProfileToggleButtonWidth,
        top + height
    };
}

RECT mappingToggleButtonRectForClient(
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    const RECT profileRect = profileToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (profileRect.right <= profileRect.left || profileRect.bottom <= profileRect.top) {
        return RECT{0, 0, 0, 0};
    }

    const int top = profileRect.bottom + 8;
    if (contentBottom <= top + 48) {
        return RECT{0, 0, 0, 0};
    }

    int height = kPanelMappingToggleButtonHeight;
    const int availableHeight = contentBottom - top - 12;
    if (height > availableHeight) {
        height = availableHeight;
    }
    if (height < 48) {
        return RECT{0, 0, 0, 0};
    }

    return RECT{
        profileRect.left,
        top,
        profileRect.right,
        top + height
    };
}

RECT editToggleButtonRectForClient(
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    const RECT mappingRect = mappingToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (mappingRect.right <= mappingRect.left || mappingRect.bottom <= mappingRect.top) {
        return RECT{0, 0, 0, 0};
    }

    const int top = mappingRect.bottom + 8;
    if (contentBottom <= top + 48) {
        return RECT{0, 0, 0, 0};
    }

    int height = kPanelEditToggleButtonHeight;
    const int availableHeight = contentBottom - top - 12;
    if (height > availableHeight) {
        height = availableHeight;
    }
    if (height < 48) {
        return RECT{0, 0, 0, 0};
    }

    return RECT{mappingRect.left, top, mappingRect.right, top + height};
}

ProfilePanelLayout profilePanelLayoutForClient(
    const bool profilePanelVisible,
    const int requestedWidth,
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    ProfilePanelLayout layout;
    if (!profilePanelVisible || clientWidth <= kPanelProfileToggleRailWidth || clientHeight <= 0) {
        return layout;
    }

    const int panelRight = clientWidth - kPanelProfileToggleRailWidth;
    const int panelWidth = clampProfilePanelWidthForClient(requestedWidth, clientWidth);
    const int panelLeft = panelRight - panelWidth;
    const int splitterLeft = panelLeft - kPanelProfileSplitterWidth;
    if (splitterLeft < kPanelDeviceToggleRailWidth || contentBottom <= kPanelTopBarHeight) {
        return layout;
    }

    layout.panelRect = RECT{panelLeft, kPanelTopBarHeight, panelRight, contentBottom};
    layout.splitterRect = RECT{splitterLeft, kPanelTopBarHeight, panelLeft, contentBottom};
    layout.valid = true;
    return layout;
}

ProfilePanelLayout profilePanelLayoutForClient(
    const bool profilePanelVisible,
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept
{
    return profilePanelLayoutForClient(
        profilePanelVisible,
        kPanelProfilePanelDefaultWidth,
        contentBottom,
        clientWidth,
        clientHeight
    );
}

} // namespace ovtr::win32
