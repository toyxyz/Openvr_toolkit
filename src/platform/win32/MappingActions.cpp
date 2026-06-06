#include "platform/win32/MappingActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/MappingActorActions.h"
#include "platform/win32/MappingActorLayout.h"
#include "platform/win32/MappingCalibrationActions.h"
#include "platform/win32/MappingDropdownActions.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingNameEditor.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/MappingPresetActions.h"
#include "platform/win32/MappingSoftIkFilter.h"
#include "platform/win32/WindowLayout.h"

#include <array>
#include <cstddef>
#include <commdlg.h>

namespace ovtr::win32 {
namespace {

void refreshMappingUi(HWND hwnd)
{
    InvalidateRect(hwnd, nullptr, FALSE);
}

void refreshMappingViewport(HWND hwnd, const AppWindowState& state)
{
    InvalidateRect(hwnd, nullptr, FALSE);
    if (state.glWindow) {
        InvalidateRect(state.glWindow, nullptr, FALSE);
        UpdateWindow(state.glWindow);
    }
}

COLORREF colorRefFromRgbColor(const RgbColor color) noexcept
{
    const RgbColor clamped = clampRgbColor(color);
    return RGB(clamped.r, clamped.g, clamped.b);
}

RgbColor rgbColorFromColorRef(const COLORREF color) noexcept
{
    return RgbColor{
        static_cast<int>(GetRValue(color)),
        static_cast<int>(GetGValue(color)),
        static_cast<int>(GetBValue(color))
    };
}

RgbColor currentMappingSkeletonColor(const AppWindowState& state) noexcept
{
    const MappingActor* actor = selectedMappingActor(state);
    return actor != nullptr ? actor->skeletonColor : state.mappingSkeletonColor;
}

bool chooseMappingSkeletonColor(HWND hwnd, AppWindowState& state)
{
    static std::array<COLORREF, 16> customColors{};
    if (customColors[0] == 0) {
        customColors.fill(RGB(255, 255, 255));
    }
    CHOOSECOLORW chooser{};
    chooser.lStructSize = sizeof(chooser);
    chooser.hwndOwner = hwnd;
    chooser.rgbResult = colorRefFromRgbColor(currentMappingSkeletonColor(state));
    chooser.lpCustColors = customColors.data();
    chooser.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (!ChooseColorW(&chooser)) {
        return true;
    }
    state.mappingSkeletonColor = rgbColorFromColorRef(chooser.rgbResult);
    state.mappingSkeletonColorCustomized = true;
    syncSelectedMappingActorFromControls(state);
    appendDebugLog(state, L"Mapping skeleton color updated");
    refreshMappingViewport(hwnd, state);
    return true;
}

} // namespace

bool handleMappingPanelClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    if (!state.mappingPanelVisible) {
        return false;
    }

    const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(&state, clientWidth, clientHeight);
    const MappingPanelControlsLayout controls = mappingControlsLayoutForPanel(panelLayout);
    if (!controls.valid) {
        return false;
    }

    if (selectMappingProfileDropdownOption(hwnd, state, panelLayout, controls, point)) {
        return true;
    }
    if (selectMappingSoftIkFilterDropdownOption(hwnd, state, controls, point)) {
        return true;
    }
    if (selectMappingDeviceDropdownOption(hwnd, state, panelLayout, point)) {
        return true;
    }
    if (selectMappingPresetDropdownOption(hwnd, state, panelLayout, controls, point)) {
        return true;
    }

    if (PtInRect(&controls.profileValueRect, point)) {
        state.mappingProfileDropdownOpen = !state.mappingProfileDropdownOpen;
        state.mappingDropdownSlot = -1;
        state.mappingPresetDropdownOpen = false;
        refreshMappingUi(hwnd);
        return true;
    }
    if (PtInRect(&controls.presetSaveButtonRect, point)) {
        saveCurrentMappingPreset(hwnd, state);
        return true;
    }
    if (PtInRect(&controls.presetValueRect, point)) {
        state.mappingPresetDropdownOpen = !state.mappingPresetDropdownOpen;
        state.mappingProfileDropdownOpen = false;
        state.mappingDropdownSlot = -1;
        refreshMappingUi(hwnd);
        return true;
    }
    if (PtInRect(&controls.colorSwatchRect, point) || PtInRect(&controls.colorPickButtonRect, point)) {
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        return chooseMappingSkeletonColor(hwnd, state);
    }
    if (toggleMappingSoftIkFilterDropdown(hwnd, state, controls, point)) {
        return true;
    }
    if (PtInRect(&controls.addActorButtonRect, point)) {
        addMappingActorFromProfile(state, state.profile, visibleMappingActorRowCount(controls));
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        appendDebugLog(state, L"Mapping actor added: " + state.profile.name);
        refreshMappingViewport(hwnd, state);
        return true;
    }

    const MappingActorRowLayout actorRow = mappingActorRowAtPoint(
        controls,
        state.mappingActors.size(),
        state.mappingActorScrollOffset,
        point
    );
    if (actorRow.valid) {
        toggleMappingActorSelectionAtIndex(state, actorRow.actorIndex);
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        refreshMappingUi(hwnd);
        return true;
    }
    if (PtInRect(&controls.calibrateButtonRect, point)) {
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        calibrateSelectedMappingActor(hwnd, state);
        return true;
    }

    const MappingPanelRowLayout clickedRow =
        mappingRowLayoutAtPoint(panelLayout, point, state.mappingScrollOffset);
    if (clickedRow.valid) {
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        state.mappingDropdownSlot = state.mappingDropdownSlot == clickedRow.slotIndex
            ? -1
            : clickedRow.slotIndex;
        refreshMappingUi(hwnd);
        return true;
    }

    if (state.mappingDropdownSlot >= 0 ||
        state.mappingProfileDropdownOpen ||
        state.mappingPresetDropdownOpen) {
        state.mappingDropdownSlot = -1;
        state.mappingProfileDropdownOpen = false;
        state.mappingPresetDropdownOpen = false;
        refreshMappingUi(hwnd);
        return PtInRect(&panelLayout.panelRect, point);
    }
    return PtInRect(&panelLayout.panelRect, point);
}

bool deleteMappingActorAtIndex(HWND hwnd, AppWindowState& state, const std::size_t actorIndex)
{
    if (actorIndex >= state.mappingActors.size()) {
        return false;
    }
    const std::wstring name = effectiveMappingActorName(state.mappingActors[actorIndex]);
    const std::uint32_t deletedId = state.mappingActors[actorIndex].id;
    state.mappingActors.erase(state.mappingActors.begin() + static_cast<std::ptrdiff_t>(actorIndex));
    if (state.selectedMappingActorId == deletedId) {
        state.selectedMappingActorId = 0;
        state.selectedMappingOffsetSlot = -1;
        state.mappingEditStepDropdownOpen = false;
        state.mappingEditOffsetPresetDropdownOpen = false;
    }
    if (state.mappingActorScrollOffset > 0 && state.mappingActors.size() <= static_cast<std::size_t>(state.mappingActorScrollOffset)) {
        state.mappingActorScrollOffset = state.mappingActors.empty() ? 0 : static_cast<int>(state.mappingActors.size() - 1);
    }
    appendDebugLog(state, L"Mapping actor deleted: " + name);
    refreshMappingViewport(hwnd, state);
    return true;
}

bool resetMappingActorAtIndex(HWND hwnd, AppWindowState& state, const std::size_t actorIndex)
{
    if (actorIndex >= state.mappingActors.size()) {
        return false;
    }
    MappingActor& actor = state.mappingActors[actorIndex];
    const std::wstring name = effectiveMappingActorName(actor);
    resetMappingActorCalibration(actor);
    if (state.selectedMappingActorId == actor.id) {
        state.selectedMappingOffsetSlot = -1;
        state.mappingEditStepDropdownOpen = false;
        state.mappingEditOffsetPresetDropdownOpen = false;
    }
    appendDebugLog(state, L"Mapping actor reset: " + name);
    refreshMappingViewport(hwnd, state);
    return true;
}

bool handleMappingPanelDoubleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    if (!state.mappingPanelVisible) {
        return false;
    }
    const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(&state, clientWidth, clientHeight);
    const MappingPanelControlsLayout controls = mappingControlsLayoutForPanel(panelLayout);
    if (!controls.valid) {
        return false;
    }
    state.mappingDropdownSlot = -1;
    state.mappingProfileDropdownOpen = false;
    state.mappingPresetDropdownOpen = false;
    if (PtInRect(&controls.actorNameValueRect, point)) {
        showMappingActorNameEditor(hwnd, state);
        return true;
    }
    if (PtInRect(&controls.nameValueRect, point)) {
        showMappingNameEditor(hwnd, state);
        return true;
    }
    return false;
}

} // namespace ovtr::win32
