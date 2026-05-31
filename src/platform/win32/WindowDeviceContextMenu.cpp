#include "platform/win32/WindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MarkerList.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/Menus.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/Win32MenuResources.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowDeviceContextMenuActions.h"
#include "platform/win32/WindowDeviceContextMenuIds.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <array>
#include <cstdint>

namespace ovtr::win32 {
namespace {

bool showMarkerContextMenu(HWND hwnd, AppWindowState& state, const MarkerListLayout& layout, const POINT point)
{
    const std::uint32_t markerId = markerIdFromListPoint(state, layout, point);
    if (markerId == kNoSelectedMarkerId) {
        return false;
    }

    selectMarker(state, markerId);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);

    UniqueMenu menu(CreatePopupMenu());
    if (!menu) {
        return false;
    }
    std::array<PopupMenuItem, 2> menuItems{{
        PopupMenuItem{kMarkerContextMenuRenameId, L"Rename"},
        PopupMenuItem{kMarkerContextMenuDeleteId, L"Delete"},
    }};
    appendPopupMenuItem(menu.get(), menuItems[0]);
    AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    appendPopupMenuItem(menu.get(), menuItems[1]);

    POINT screenPoint = point;
    ClientToScreen(hwnd, &screenPoint);
    SetForegroundWindow(hwnd);
    const UINT command = TrackPopupMenu(
        menu.get(),
        TPM_RIGHTBUTTON | TPM_RETURNCMD,
        screenPoint.x,
        screenPoint.y,
        0,
        hwnd,
        nullptr
    );
    executeMarkerContextMenuCommand(hwnd, state, command);
    return true;
}

bool showDeviceContextMenu(HWND hwnd, AppWindowState& state, const DeviceListLayout& layout, const POINT point)
{
    if (!layout.valid || !PtInRect(&layout.boxRect, point)) {
        return false;
    }

    const std::uint32_t clickedRuntimeIndex =
        deviceRuntimeIndexFromListPoint(state, layout, point);
    if (clickedRuntimeIndex != kNoSelectedRuntimeIndex) {
        const ovtr::DeviceDescriptor* clickedDevice = deviceForRuntimeIndex(
            state.devices,
            clickedRuntimeIndex
        );
        if (clickedDevice && state.selectedDeviceRuntimeIndex != clickedDevice->runtimeIndex) {
            state.selectedDeviceRuntimeIndex = clickedDevice->runtimeIndex;
            appendDebugLog(state, L"Device selected: " + widen(deviceDisplayName(*clickedDevice)));
            if (state.glWindow) {
                renderViewport(state.glWindow);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    if (!selectedListDevice(state)) {
        return false;
    }

    UniqueMenu menu(CreatePopupMenu());
    if (!menu) {
        return false;
    }

    std::array<PopupMenuItem, 3> menuItems{{
        PopupMenuItem{kDeviceContextMenuSetNameId, L"Set Name"},
        PopupMenuItem{kDeviceContextMenuAddMarkerId, L"Add marker"},
        PopupMenuItem{kDeviceContextMenuSetOriginId, L"Set to Origin"},
    }};
    appendPopupMenuItem(menu.get(), menuItems[0]);
    AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    appendPopupMenuItem(menu.get(), menuItems[1]);
    appendPopupMenuItem(menu.get(), menuItems[2]);

    POINT screenPoint = point;
    ClientToScreen(hwnd, &screenPoint);
    SetForegroundWindow(hwnd);
    const UINT command = TrackPopupMenu(
        menu.get(),
        TPM_RIGHTBUTTON | TPM_RETURNCMD,
        screenPoint.x,
        screenPoint.y,
        0,
        hwnd,
        nullptr
    );
    executeDeviceContextMenuCommand(hwnd, state, command);
    return true;
}

bool showMappingActorContextMenu(HWND hwnd, AppWindowState& state, const POINT point, const int width, const int height)
{
    if (!state.mappingPanelVisible) {
        return false;
    }
    const ProfilePanelLayout panel = profilePanelLayoutForClient(&state, width, height);
    const MappingPanelControlsLayout controls = mappingControlsLayoutForPanel(panel);
    const MappingActorRowLayout row =
        mappingActorRowAtPoint(controls, state.mappingActors.size(), state.mappingActorScrollOffset, point);
    if (!row.valid) {
        return false;
    }
    state.selectedMappingActorId = state.mappingActors[row.actorIndex].id;
    InvalidateRect(hwnd, &controls.actorListRect, FALSE);

    UniqueMenu menu(CreatePopupMenu());
    if (!menu) {
        return false;
    }
    PopupMenuItem resetItem{kMappingActorContextMenuResetId, L"Reset"};
    PopupMenuItem deleteItem{kMappingActorContextMenuDeleteId, L"Delete"};
    appendPopupMenuItem(menu.get(), resetItem);
    AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    appendPopupMenuItem(menu.get(), deleteItem);

    POINT screenPoint = point;
    ClientToScreen(hwnd, &screenPoint);
    SetForegroundWindow(hwnd);
    const UINT command = TrackPopupMenu(
        menu.get(),
        TPM_RIGHTBUTTON | TPM_RETURNCMD,
        screenPoint.x,
        screenPoint.y,
        0,
        hwnd,
        nullptr
    );
    if (command == kMappingActorContextMenuResetId) {
        resetMappingActorAtIndex(hwnd, state, row.actorIndex);
    } else if (command == kMappingActorContextMenuDeleteId) {
        deleteMappingActorAtIndex(hwnd, state, row.actorIndex);
    }
    return true;
}

} // namespace

bool handleMainWindowRButtonDown(HWND hwnd, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return false;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

    if (showMappingActorContextMenu(hwnd, *state, point, clientWidth, clientHeight)) {
        return true;
    }

    const MarkerListLayout markerListLayout = markerListLayoutForClient(state, clientWidth, clientHeight);
    if (markerListLayout.valid && PtInRect(&markerListLayout.boxRect, point)) {
        return showMarkerContextMenu(hwnd, *state, markerListLayout, point);
    }

    const DeviceListLayout deviceListLayout = deviceListLayoutForClient(state, clientWidth, clientHeight);
    return showDeviceContextMenu(hwnd, *state, deviceListLayout, point);
}

} // namespace ovtr::win32
