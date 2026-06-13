#include "platform/win32/WindowInput.h"

#include <windowsx.h>

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingEditPanelLayout.h"
#include "platform/win32/MappingNameEditor.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/ProfileEditor.h"
#include "platform/win32/ProfilePanelLayout.h"
#include "platform/win32/RecordingSessionList.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <vector>

namespace ovtr::win32 {

bool handleMainWindowMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return false;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    ScreenToClient(hwnd, &point);
    const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
    int wheelSteps = wheelDelta / WHEEL_DELTA;
    if (wheelSteps == 0) {
        wheelSteps = wheelDelta > 0 ? 1 : -1;
    }

    const ProfilePanelControlsLayout profileControls =
        profileControlsLayoutForPanel(profilePanelLayoutForClient(state, clientWidth, clientHeight));
    if (state->profilePanelVisible && profileControls.valid && PtInRect(&profileControls.tableRect, point)) {
        state->profileScrollOffset -= wheelSteps * 3;
        state->profileScrollOffset = clampProfileScrollOffset(
            state->profileScrollOffset,
            profileControls.visibleRowCount
        );
        updateProfileEditorLayout(hwnd, *state);
        InvalidateRect(hwnd, &profileControls.tableRect, FALSE);
        return true;
    }

    const MappingPanelControlsLayout mappingControls =
        mappingControlsLayoutForPanel(profilePanelLayoutForClient(state, clientWidth, clientHeight));
    if (state->mappingPanelVisible && mappingControls.valid && PtInRect(&mappingControls.tableRect, point)) {
        state->mappingScrollOffset -= wheelSteps * 3;
        state->mappingScrollOffset = clampMappingScrollOffset(
            state->mappingScrollOffset,
            mappingControls.visibleRowCount
        );
        state->mappingDropdownSlot = -1;
        state->mappingProfileDropdownOpen = false;
        state->mappingPresetDropdownOpen = false;
        updateMappingActorNameEditorLayout(hwnd, *state);
        updateMappingNameEditorLayout(hwnd, *state);
        const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(state, clientWidth, clientHeight);
        InvalidateRect(hwnd, &panelLayout.panelRect, FALSE);
        return true;
    }
    if (state->mappingPanelVisible && mappingControls.valid && PtInRect(&mappingControls.actorListRect, point)) {
        state->mappingActorScrollOffset -= wheelSteps * 3;
        state->mappingActorScrollOffset = clampMappingActorScrollOffset(
            state->mappingActorScrollOffset,
            state->mappingActors.size(),
            visibleMappingActorRowCount(mappingControls)
        );
        InvalidateRect(hwnd, &mappingControls.actorListRect, FALSE);
        return true;
    }

    const ProfilePanelLayout editPanelLayout = profilePanelLayoutForClient(state, clientWidth, clientHeight);
    const MappingEditPanelLayout editLayout = mappingEditPanelLayoutForPanel(editPanelLayout);
    if (state->editPanelVisible && editLayout.valid && PtInRect(&editLayout.listRect, point)) {
        state->mappingEditOffsetScrollOffset -= wheelSteps * 3;
        state->mappingEditOffsetScrollOffset = clampMappingEditOffsetScrollOffset(
            state->mappingEditOffsetScrollOffset,
            editLayout.visibleRowCount
        );
        InvalidateRect(hwnd, &editLayout.listRect, FALSE);
        return true;
    }

    const DeviceListLayout deviceListLayout = deviceListLayoutForClient(state, clientWidth, clientHeight);
    if (deviceListLayout.valid && PtInRect(&deviceListLayout.boxRect, point)) {
        state->deviceListScrollOffset -= wheelSteps * 3;
        clampDeviceListScroll(*state, deviceListLayout.visibleItemCount);
        InvalidateRect(hwnd, &deviceListLayout.boxRect, FALSE);
        return true;
    }

    const std::vector<RecordingSessionListRow>& sessionRows =
        cachedRecordingSessionFolders(*state, activeSessionDirectoryPath(*state));
    const SessionListLayout sessionListLayout = sessionListLayoutForClient(
        state,
        clientWidth,
        clientHeight,
        static_cast<int>(sessionRows.size())
    );
    if (sessionListLayout.valid && PtInRect(&sessionListLayout.boxRect, point)) {
        state->sessionListScrollOffset -= wheelSteps * 3;
        clampSessionListScroll(
            *state,
            static_cast<int>(sessionRows.size()),
            sessionListLayout.visibleItemCount
        );
        InvalidateRect(hwnd, &sessionListLayout.boxRect, FALSE);
        return true;
    }

    const MarkerListLayout markerListLayout = markerListLayoutForClient(state, clientWidth, clientHeight);
    if (markerListLayout.valid && PtInRect(&markerListLayout.boxRect, point)) {
        state->markerListScrollOffset -= wheelSteps * 3;
        clampMarkerListScroll(*state, markerListLayout.visibleItemCount);
        InvalidateRect(hwnd, &markerListLayout.boxRect, FALSE);
        return true;
    }

    const RECT debugInfoRect = debugInfoRectForClient(state, clientWidth, clientHeight);
    if (state->debugMonitorVisible && PtInRect(&debugInfoRect, point)) {
        const int visibleLineCount = visibleDebugLogLineCount(debugInfoRect);
        state->debugInfoScrollOffset -= wheelSteps * 3;
        clampDebugInfoScroll(*state, visibleLineCount);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    const RECT messagesRect = debugMessagesRectForClient(state, clientWidth, clientHeight);
    if (state->debugMonitorVisible && PtInRect(&messagesRect, point)) {
        const int visibleLineCount = visibleDebugLogLineCount(messagesRect);
        state->debugLogScrollOffset += wheelSteps * 3;
        clampDebugLogScroll(*state, visibleLineCount);
        InvalidateRect(hwnd, nullptr, FALSE);
        return true;
    }

    return false;
}

} // namespace ovtr::win32
