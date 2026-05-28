#include "platform/win32/Layout.h"

#include "platform/win32/PanelLayoutMetrics.h"

namespace ovtr::win32 {

int defaultLeftPanelWidthForClient(const int clientWidth) noexcept
{
    const int proportional = static_cast<int>(
        static_cast<float>(clientWidth) * kDefaultLeftPanelWidthRatio
    );
    if (proportional < kDefaultLeftPanelMinWidth) {
        return kDefaultLeftPanelMinWidth;
    }
    if (proportional > kDefaultLeftPanelMaxWidth) {
        return kDefaultLeftPanelMaxWidth;
    }
    return proportional;
}

int clampLeftPanelWidthForClient(const int requestedWidth, const int clientWidth) noexcept
{
    const int maxByViewport = clientWidth - kPanelSplitterWidth - kPanelContentMargin - kViewportMinWidth;
    if (maxByViewport <= 0) {
        return clientWidth > kPanelSplitterWidth ? clientWidth - kPanelSplitterWidth : 0;
    }

    int maxWidth = maxByViewport < kLeftPanelMaxWidth ? maxByViewport : kLeftPanelMaxWidth;
    if (maxWidth < kLeftPanelMinWidth) {
        maxWidth = maxByViewport;
    }

    int minWidth = kLeftPanelMinWidth;
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

int leftPanelWidthForClient(
    const bool devicePanelVisible,
    const int requestedWidth,
    const int clientWidth
) noexcept
{
    if (!devicePanelVisible) {
        return clientWidth > kPanelDeviceToggleRailWidth ? kPanelDeviceToggleRailWidth : clientWidth;
    }

    const int width = requestedWidth > 0 ? requestedWidth : defaultLeftPanelWidthForClient(clientWidth);
    return clampLeftPanelWidthForClient(width, clientWidth);
}

} // namespace ovtr::win32
