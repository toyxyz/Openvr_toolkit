#include "platform/win32/WindowClickActionSections.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/MappingNameEditor.h"
#include "platform/win32/MappingOffsetPresetNameEditor.h"
#include "platform/win32/ProfileActions.h"
#include "platform/win32/ProfileEditor.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleEditToggleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    const RECT editButtonRect = editToggleButtonRectForClient(&state, clientWidth, clientHeight);
    if (!PtInRect(&editButtonRect, point)) {
        return false;
    }

    state.editPanelVisible = !state.editPanelVisible;
    state.mappingEditStepDropdownOpen = false;
    state.mappingEditOffsetPresetDropdownOpen = false;
    closeMappingOffsetPresetNameEditor(hwnd, state);
    if (state.editPanelVisible) {
        state.profilePanelVisible = false;
        state.mappingPanelVisible = false;
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        disableProfilePreview(state);
        closeProfileEditor(hwnd, state);
        closeMappingActorNameEditor(hwnd, state);
        closeMappingNameEditor(hwnd, state);
    } else {
        state.selectedMappingOffsetSlot = -1;
    }
    state.profileSplitterDragging = false;
    appendDebugLog(
        state,
        state.editPanelVisible ? L"Edit panel opened" : L"Edit panel closed"
    );
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

} // namespace ovtr::win32
