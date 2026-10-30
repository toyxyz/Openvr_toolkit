#include "platform/win32/Layout.h"

#include "platform/win32/DeviceListLayoutMetrics.h"
#include "platform/win32/PanelLayoutMetrics.h"

#include <algorithm>

namespace ovtr::win32 {
namespace {

constexpr int kCameraRowHeight = 34;

} // namespace

RECT videoToggleButtonRectForClient(
    const int contentBottom,
    const int clientWidth,
    const int clientHeight
) noexcept {
    const RECT previous = streamingToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
    if (previous.bottom <= previous.top) {
        return RECT{0, 0, 0, 0};
    }
    const int top = previous.bottom + 8;
    const int wantedBottom = top + kPanelVideoToggleButtonHeight;
    const int bottom = wantedBottom < contentBottom - 12 ? wantedBottom : contentBottom - 12;
    if (bottom <= top) {
        return RECT{0, 0, 0, 0};
    }
    const int left = (kPanelDeviceToggleRailWidth - kPanelDeviceToggleButtonWidth) / 2;
    return RECT{left, top, left + kPanelDeviceToggleButtonWidth, bottom};
}

VideoPanelLayout videoPanelLayoutForClient(
    const bool visible,
    const int leftPanelWidth,
    const int contentBottom,
    const int cameraCount
) noexcept {
    VideoPanelLayout layout;
    if (!visible) {
        return layout;
    }
    const int left = kDeviceListToggleRailWidth + kDeviceListContentMargin - 8;
    const int right = leftPanelWidth - 24;
    const int top = kDeviceListTopBarHeight + 12;
    const int bottom = contentBottom - 12;
    if (right <= left + 180 || bottom <= top + 240) {
        return layout;
    }
    layout.boxRect = RECT{left, top, right, bottom};
    layout.headerRect = RECT{left + 12, top + 10, right - 12, top + 38};
    layout.refreshButtonRect = RECT{left + 12, layout.headerRect.bottom + 6,
        left + 132, layout.headerRect.bottom + 40};
    layout.statusRect = RECT{layout.refreshButtonRect.right + 10, layout.refreshButtonRect.top,
        right - 12, layout.refreshButtonRect.bottom};
    const int listTop = layout.refreshButtonRect.bottom + 10;
    const int desiredRows = std::clamp(cameraCount, 2, 5);
    layout.cameraListRect = RECT{left + 12, listTop, right - 12,
        listTop + desiredRows * kCameraRowHeight};
    layout.visibleCameraCount = desiredRows;
    layout.previewRect = RECT{left + 12, layout.cameraListRect.bottom + 12,
        right - 12, bottom - 12};
    if (layout.previewRect.bottom <= layout.previewRect.top + 80) {
        return VideoPanelLayout{};
    }
    layout.valid = true;
    return layout;
}

int videoCameraIndexFromPoint(
    const VideoPanelLayout& layout,
    const POINT point,
    const int cameraCount
) noexcept {
    if (!layout.valid || !PtInRect(&layout.cameraListRect, point)) {
        return -1;
    }
    const int index = (point.y - layout.cameraListRect.top) / kCameraRowHeight;
    return index >= 0 && index < cameraCount && index < layout.visibleCameraCount ? index : -1;
}

} // namespace ovtr::win32
