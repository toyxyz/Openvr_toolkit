#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppState.h"

namespace ovtr::win32 {

RECT deviceToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    return deviceToggleButtonRectForClient(contentBottom, clientWidth, clientHeight);
}

RECT splitterRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const int leftPanelWidth = leftPanelWidthForClient(state, clientWidth);
    return splitterRectForClient(
        leftPanelWidth,
        activeDebugMonitorHeight(state, clientHeight),
        clientHeight
    );
}

DeviceListLayout deviceListLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state) {
        return {};
    }

    const int leftPanelWidth = leftPanelWidthForClient(state, clientWidth);
    const int contentBottom = leftPanelContentBottomForClient(state, clientHeight);
    const OriginPanelLayout originLayout = originPanelLayoutForClient(state, clientWidth, clientHeight);
    return deviceListLayoutForClient(
        state->devicePanelVisible,
        leftPanelWidth,
        contentBottom,
        originLayout.valid,
        originLayout.boxRect.top,
        static_cast<int>(state->devices.size())
    );
}

int maxDeviceListScrollOffset(const AppWindowState& state, const int visibleItemCount)
{
    return maxDeviceListScrollOffset(
        static_cast<int>(state.devices.size()),
        visibleItemCount
    );
}

void clampDeviceListScroll(AppWindowState& state, const int visibleItemCount)
{
    state.deviceListScrollOffset = clampDeviceListScrollOffset(
        state.deviceListScrollOffset,
        static_cast<int>(state.devices.size()),
        visibleItemCount
    );
}

std::uint32_t deviceRuntimeIndexFromListPoint(
    const AppWindowState& state,
    const DeviceListLayout& layout,
    const POINT point
)
{
    return deviceRuntimeIndexFromListPoint(
        static_cast<const AppRuntimeState&>(state),
        static_cast<const AppDeviceState&>(state),
        layout,
        point
    );
}

int leftPanelWidthForClient(const AppWindowState* state, const int clientWidth)
{
    const int requestedWidth = state && state->leftPanelWidth > 0
        ? state->leftPanelWidth
        : 0;
    return leftPanelWidthForClient(
        !state || state->devicePanelVisible,
        requestedWidth,
        clientWidth
    );
}

} // namespace ovtr::win32
