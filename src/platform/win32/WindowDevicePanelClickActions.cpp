#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MarkerList.h"
#include "platform/win32/MappingNameEditor.h"
#include "platform/win32/OriginActions.h"
#include "platform/win32/OriginEditor.h"
#include "platform/win32/ProfileEditor.h"
#include "platform/win32/SessionEditor.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"

#include <cstdint>

namespace ovtr::win32 {

bool handleDeviceToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT deviceButtonRect = deviceToggleButtonRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&deviceButtonRect, point)) {
        return false;
    }

    state.devicePanelVisible = !state.devicePanelVisible;
    state.splitterDragging = false;
    if (!state.devicePanelVisible && state.originEditWindow) {
        closeOriginEditor(hwnd, state);
    }
    if (!state.devicePanelVisible && state.sessionEditWindow) {
        closeSessionEditor(hwnd, state);
    }
    appendDebugLog(
        state,
        state.devicePanelVisible ? L"Device panel opened" : L"Device panel closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

bool handleProfileToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT profileButtonRect = profileToggleButtonRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&profileButtonRect, point)) {
        return false;
    }

    state.profilePanelVisible = !state.profilePanelVisible;
    if (state.profilePanelVisible) {
        state.mappingPanelVisible = false;
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        closeMappingNameEditor(hwnd, state);
    }
    state.profileSplitterDragging = false;
    if (!state.profilePanelVisible) {
        disableProfilePreview(state);
    }
    if (!state.profilePanelVisible && state.profileEditWindow) {
        closeProfileEditor(hwnd, state);
    }
    appendDebugLog(
        state,
        state.profilePanelVisible ? L"Profile panel opened" : L"Profile panel closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

bool handleMappingToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT mappingButtonRect = mappingToggleButtonRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&mappingButtonRect, point)) {
        return false;
    }

    state.mappingPanelVisible = !state.mappingPanelVisible;
    if (state.mappingPanelVisible) {
        state.profilePanelVisible = false;
        disableProfilePreview(state);
        closeProfileEditor(hwnd, state);
        if (state.mappingPresetName.empty()) {
            state.mappingPresetName = state.profile.name;
        }
    }
    state.mappingDropdownSlot = -1;
    state.mappingProfileDropdownOpen = false;
    state.mappingPresetDropdownOpen = false;
    if (!state.mappingPanelVisible) {
        closeMappingNameEditor(hwnd, state);
    }
    state.profileSplitterDragging = false;
    appendDebugLog(
        state,
        state.mappingPanelVisible ? L"Mapping panel opened" : L"Mapping panel closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

bool handleProfileSplitterClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT splitterRect = profileSplitterRectForClient(&state, clientWidth, clientHeight);
    if (!(state.profilePanelVisible || state.mappingPanelVisible) || !PtInRect(&splitterRect, point)) {
        return false;
    }

    state.profileSplitterDragging = true;
    state.profilePanelWidth = state.profilePanelWidth > 0
        ? clampProfilePanelWidthForClient(state.profilePanelWidth, clientWidth)
        : defaultProfilePanelWidthForClient(clientWidth);
    SetCapture(hwnd);
    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

bool handleDeviceSplitterClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT splitterRect = splitterRectForClient(&state, clientWidth, clientHeight);
    if (!state.devicePanelVisible || !PtInRect(&splitterRect, point)) {
        return false;
    }

    state.splitterDragging = true;
    state.leftPanelWidth = leftPanelWidthForClient(&state, clientWidth);
    SetCapture(hwnd);
    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

bool handleDeviceListClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const DeviceListLayout deviceListLayout = deviceListLayoutForClient(&state, clientWidth, clientHeight);
    const std::uint32_t clickedRuntimeIndex =
        deviceRuntimeIndexFromListPoint(state, deviceListLayout, point);
    if (clickedRuntimeIndex == kNoSelectedRuntimeIndex) {
        return false;
    }

    const ovtr::DeviceDescriptor* clickedDevice = deviceForRuntimeIndex(
        state.devices,
        clickedRuntimeIndex
    );
    if (!clickedDevice) {
        return false;
    }
    toggleListDeviceSelection(state, *clickedDevice);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

bool handleMarkerListClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const MarkerListLayout markerListLayout = markerListLayoutForClient(&state, clientWidth, clientHeight);
    const std::uint32_t clickedMarkerId = markerIdFromListPoint(state, markerListLayout, point);
    if (clickedMarkerId == kNoSelectedMarkerId) {
        return false;
    }

    toggleMarkerSelection(state, clickedMarkerId);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

} // namespace ovtr::win32
