#include "platform/win32/MappingDropdownActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/ProfileStore.h"
#include "platform/win32/Win32String.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ovtr::win32 {
namespace {

void refreshMappingUi(HWND hwnd)
{
    InvalidateRect(hwnd, nullptr, FALSE);
}

void refreshMappingViewport(HWND hwnd, const AppWindowState& state)
{
    refreshMappingUi(hwnd);
    if (state.glWindow) {
        InvalidateRect(state.glWindow, nullptr, FALSE);
        UpdateWindow(state.glWindow);
    }
}

void showMappingError(HWND hwnd, const std::string& error)
{
    MessageBoxW(hwnd, widen(error).c_str(), L"Mapping", MB_OK | MB_ICONERROR);
}

} // namespace

bool selectMappingProfileDropdownOption(
    HWND hwnd,
    AppWindowState& state,
    const ProfilePanelLayout& panelLayout,
    const MappingPanelControlsLayout& controls,
    const POINT point
) {
    if (!state.mappingProfileDropdownOpen) {
        return false;
    }
    std::vector<ProfileFileEntry> profiles;
    std::string error;
    if (!listSavedProfiles(profiles, error)) {
        state.mappingProfileDropdownOpen = false;
        showMappingError(hwnd, error);
        refreshMappingUi(hwnd);
        return true;
    }
    const int option = mappingProfileDropdownOptionFromPoint(
        controls,
        panelLayout,
        static_cast<int>(profiles.size()),
        point
    );
    if (option < 0) {
        return false;
    }
    if (option >= static_cast<int>(profiles.size())) {
        state.mappingProfileDropdownOpen = false;
        refreshMappingUi(hwnd);
        return true;
    }
    BodyProfile loaded;
    if (!loadProfileFromPath(profiles[static_cast<std::size_t>(option)].path, loaded, error)) {
        state.mappingProfileDropdownOpen = false;
        showMappingError(hwnd, error);
        refreshMappingUi(hwnd);
        return true;
    }
    state.profile = std::move(loaded);
    syncSelectedMappingActorFromControls(state);
    state.mappingProfileDropdownOpen = false;
    state.mappingPresetDropdownOpen = false;
    appendDebugLog(state, L"Mapping profile selected: " + state.profile.name);
    refreshMappingViewport(hwnd, state);
    return true;
}

bool selectMappingDeviceDropdownOption(
    HWND hwnd,
    AppWindowState& state,
    const ProfilePanelLayout& panelLayout,
    const POINT point
) {
    (void)hwnd;
    if (!isMappingDeviceRow(state.mappingDropdownSlot) && !isMappingFingerRow(state.mappingDropdownSlot)) {
        return false;
    }
    const MappingPanelRowLayout row =
        mappingRowLayoutForSlot(panelLayout, state.mappingDropdownSlot, state.mappingScrollOffset);
    if (!row.valid) {
        state.mappingDropdownSlot = -1;
        return false;
    }
    const int fingerSide = mappingFingerSideIndexForRow(state.mappingDropdownSlot);
    const std::vector<DeviceListRow> rows = fingerSide >= 0
        ? makeFingerInputRows(state, fingerSide)
        : makeDeviceListRows(state);
    const int option = mappingDropdownOptionFromPoint(row, panelLayout, static_cast<int>(rows.size()) + 1, point);
    if (option < 0 || option > static_cast<int>(rows.size())) {
        return false;
    }
    const std::uint32_t runtimeIndex =
        option == 0 ? kNoSelectedRuntimeIndex : rows[static_cast<std::size_t>(option - 1)].runtimeIndex;
    if (fingerSide >= 0) {
        state.mappingFingerRuntimeIndices[static_cast<std::size_t>(fingerSide)] = runtimeIndex;
    } else {
        state.mappingDeviceRuntimeIndices[static_cast<std::size_t>(state.mappingDropdownSlot)] = runtimeIndex;
    }
    syncSelectedMappingActorFromControls(state);
    state.mappingDropdownSlot = -1;
    state.mappingProfileDropdownOpen = false;
    state.mappingPresetDropdownOpen = false;
    appendDebugLog(state, L"Mapping device selected");
    refreshMappingUi(hwnd);
    return true;
}

} // namespace ovtr::win32
