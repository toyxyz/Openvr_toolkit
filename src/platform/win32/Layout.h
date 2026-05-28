#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

#include <vector>

namespace ovtr::win32 {

int maxDebugMonitorHeightForClient(int clientHeight) noexcept;
int clampDebugMonitorHeightForClient(int requestedHeight, int clientHeight) noexcept;

RECT debugButtonRectForClient(int clientWidth, int clientHeight) noexcept;
RECT debugMessagesRectForClient(int activeDebugMonitorHeight, int clientWidth, int clientHeight) noexcept;
RECT debugInfoRectForClient(int activeDebugMonitorHeight, int clientWidth, int clientHeight) noexcept;
RECT debugResizeRectForClient(int activeDebugMonitorHeight, int clientWidth, int clientHeight) noexcept;
RECT topBarSettingRectForClient(int clientWidth, int clientHeight) noexcept;
RECT topBarFileRectForClient(int clientWidth, int clientHeight) noexcept;

int visibleDebugLineCountForRect(const RECT& bodyRect) noexcept;
int maxDebugScrollOffset(int totalLineCount, int visibleLineCount) noexcept;
int clampDebugScrollOffset(int scrollOffset, int totalLineCount, int visibleLineCount) noexcept;
DebugScrollbarLayout debugScrollbarLayoutForRect(
    const RECT& bodyRect,
    int totalLineCount,
    int visibleLineCount,
    int scrollOffset,
    bool reverseScrollOrigin
) noexcept;

int defaultLeftPanelWidthForClient(int clientWidth) noexcept;
int clampLeftPanelWidthForClient(int requestedWidth, int clientWidth) noexcept;
int leftPanelWidthForClient(bool devicePanelVisible, int requestedWidth, int clientWidth) noexcept;

int contentBottomForClient(int activeDebugMonitorHeight, int clientHeight) noexcept;
RECT splitterRectForClient(int leftPanelWidth, int activeDebugMonitorHeight, int clientHeight) noexcept;
RECT deviceToggleButtonRectForClient(int contentBottom, int clientWidth, int clientHeight) noexcept;
DeviceListLayout deviceListLayoutForClient(
    bool devicePanelVisible,
    int leftPanelWidth,
    int contentBottom,
    bool originPanelValid,
    int originPanelTop,
    int deviceCount
) noexcept;
int maxDeviceListScrollOffset(int totalItemCount, int visibleItemCount) noexcept;
int clampDeviceListScrollOffset(int scrollOffset, int totalItemCount, int visibleItemCount) noexcept;
int deviceListItemTextRight(const DeviceListLayout& layout, int totalItemCount) noexcept;
int deviceListRowIndexFromPoint(
    const DeviceListLayout& layout,
    POINT point,
    int totalItemCount,
    int scrollOffset
) noexcept;
RECT originEditorRectForLayout(const OriginPanelLayout& layout) noexcept;
RECT originStepperRowRect(const OriginPanelLayout& layout, bool rotation) noexcept;
RECT originStepperRowLabelRect(const OriginPanelLayout& layout, bool rotation) noexcept;
std::vector<OriginStepperAxisLayout> originStepperAxisLayoutsForLayout(
    const OriginPanelLayout& layout,
    bool rotation
);
std::vector<OriginStepperButton> originStepperButtonsForLayout(const OriginPanelLayout& layout);
OriginStepperButton originStepperButtonFromPoint(const OriginPanelLayout& layout, POINT point);
ViewportControlLayout viewportControlLayoutForClient(
    int leftPanelWidth,
    int contentBottom,
    bool showAnimationControls,
    int clientWidth,
    int clientHeight
) noexcept;
RECT viewportRenderRectForClient(
    int leftPanelWidth,
    int contentBottom,
    const ViewportControlLayout& controls,
    int clientWidth
) noexcept;

} // namespace ovtr::win32
