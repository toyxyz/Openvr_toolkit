#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/OriginActions.h"
#include "platform/win32/OriginEditor.h"
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
    appendDebugLog(
        state,
        state.devicePanelVisible ? L"Device panel opened" : L"Device panel closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
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

} // namespace ovtr::win32
