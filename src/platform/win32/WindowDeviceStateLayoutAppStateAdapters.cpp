#include "platform/win32/WindowLayout.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppMarkerState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MarkerList.h"

#include <vector>

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

RECT profileToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    return profileToggleButtonRectForClient(
        leftPanelContentBottomForClient(state, clientHeight),
        clientWidth,
        clientHeight
    );
}

RECT mappingToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    return mappingToggleButtonRectForClient(
        leftPanelContentBottomForClient(state, clientHeight),
        clientWidth,
        clientHeight
    );
}

RECT editToggleButtonRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state || clientWidth <= 0 || clientHeight <= 0) {
        return RECT{0, 0, 0, 0};
    }

    return editToggleButtonRectForClient(
        leftPanelContentBottomForClient(state, clientHeight),
        clientWidth,
        clientHeight
    );
}

ProfilePanelLayout profilePanelLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state) {
        return {};
    }

    return profilePanelLayoutForClient(
        state->profilePanelVisible || state->mappingPanelVisible || state->editPanelVisible,
        state->profilePanelWidth > 0 ? state->profilePanelWidth : defaultProfilePanelWidthForClient(clientWidth),
        leftPanelContentBottomForClient(state, clientHeight),
        clientWidth,
        clientHeight
    );
}

RECT profileSplitterRectForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    const ProfilePanelLayout layout = profilePanelLayoutForClient(state, clientWidth, clientHeight);
    return layout.valid ? layout.splitterRect : RECT{0, 0, 0, 0};
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
    const MarkerListLayout markerLayout = markerListLayoutForClient(state, clientWidth, clientHeight);
    const SessionListLayout sessionLayout = sessionListLayoutForClient(
        state,
        clientWidth,
        clientHeight,
        0
    );
    bool lowerPanelValid = originLayout.valid;
    int lowerPanelTop = originLayout.boxRect.top;
    if (markerLayout.valid && (!lowerPanelValid || markerLayout.boxRect.top < lowerPanelTop)) {
        lowerPanelValid = true;
        lowerPanelTop = markerLayout.boxRect.top;
    }
    if (sessionLayout.valid && (!lowerPanelValid || sessionLayout.boxRect.top < lowerPanelTop)) {
        lowerPanelValid = true;
        lowerPanelTop = sessionLayout.boxRect.top;
    }
    return deviceListLayoutForClient(
        state->devicePanelVisible,
        leftPanelWidth,
        contentBottom,
        lowerPanelValid,
        lowerPanelTop,
        static_cast<int>(makeDevicePanelRows(*state).size())
    );
}

MarkerListLayout markerListLayoutForClient(
    const AppWindowState* state,
    const int clientWidth,
    const int clientHeight
)
{
    if (!state) {
        return {};
    }

    return markerListLayoutForClient(
        state->devicePanelVisible,
        leftPanelWidthForClient(state, clientWidth),
        leftPanelContentBottomForClient(state, clientHeight),
        static_cast<int>(state->markers.size())
    );
}

int maxDeviceListScrollOffset(const AppWindowState& state, const int visibleItemCount)
{
    return maxDeviceListScrollOffset(
        static_cast<int>(makeDevicePanelRows(state).size()),
        visibleItemCount
    );
}

void clampDeviceListScroll(AppWindowState& state, const int visibleItemCount)
{
    state.deviceListScrollOffset = clampDeviceListScrollOffset(
        state.deviceListScrollOffset,
        static_cast<int>(makeDevicePanelRows(state).size()),
        visibleItemCount
    );
}

int maxMarkerListScrollOffset(const AppWindowState& state, const int visibleItemCount)
{
    return maxMarkerListScrollOffset(
        static_cast<int>(state.markers.size()),
        visibleItemCount
    );
}

void clampMarkerListScroll(AppWindowState& state, const int visibleItemCount)
{
    state.markerListScrollOffset = clampMarkerListScrollOffset(
        state.markerListScrollOffset,
        static_cast<int>(state.markers.size()),
        visibleItemCount
    );
}

std::uint32_t deviceRuntimeIndexFromListPoint(
    const AppWindowState& state,
    const DeviceListLayout& layout,
    const POINT point
)
{
    const std::vector<DeviceListRow> rows = makeDevicePanelRows(state);
    const int rowIndex = deviceListRowIndexFromPoint(
        layout,
        point,
        static_cast<int>(rows.size()),
        state.deviceListScrollOffset
    );
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) {
        return kNoSelectedRuntimeIndex;
    }
    return rows[static_cast<std::size_t>(rowIndex)].runtimeIndex;
}

std::uint32_t markerIdFromListPoint(
    const AppWindowState& state,
    const MarkerListLayout& layout,
    const POINT point
)
{
    return markerIdFromListPoint(
        static_cast<const AppMarkerState&>(state),
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
        !state || state->devicePanelVisible || state->sessionPanelVisible || state->streamingPanelVisible,
        requestedWidth,
        clientWidth
    );
}

int rightProfileAreaWidthForClient(const AppWindowState* state, const int clientWidth)
{
    return rightProfileAreaWidthForClient(
        state && (state->profilePanelVisible || state->mappingPanelVisible || state->editPanelVisible),
        state && state->profilePanelWidth > 0 ? state->profilePanelWidth : defaultProfilePanelWidthForClient(clientWidth),
        clientWidth
    );
}

} // namespace ovtr::win32
