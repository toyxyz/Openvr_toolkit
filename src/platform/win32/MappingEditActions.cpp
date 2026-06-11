#include "platform/win32/MappingEditActions.h"

#include "math/QuaternionUtils.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/MappingCalibrationSolve.h"
#include "platform/win32/MappingEditPanelLayout.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingOffsetPresetNameEditor.h"
#include "platform/win32/MappingOffsetPresetStore.h"
#include "platform/win32/MappingTransformMath.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/Win32String.h"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

MappingActor* selectedActor(AppWindowState& state) noexcept
{
    for (MappingActor& actor : state.mappingActors) {
        if (actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    return nullptr;
}

void refresh(HWND hwnd, AppWindowState& state)
{
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showOffsetError(HWND hwnd, const std::string& error)
{
    MessageBoxW(hwnd, widen(error).c_str(), L"Offset", MB_OK | MB_ICONERROR);
}

std::wstring defaultOffsetPresetName(const MappingActor& actor)
{
    return sanitizedMappingOffsetPresetFileStem(actor.profile.name + L"_offsets");
}

float selectedStepMeters(const AppWindowState& state) noexcept
{
    for (const float option : kMappingEditOffsetStepOptionsMeters) {
        if (std::fabs(state.mappingEditOffsetStepMeters - option) < 0.00001f) {
            return option;
        }
    }
    return kMappingEditOffsetStepOptionsMeters.front();
}

bool confirmOffsetPresetSave(HWND hwnd, const std::wstring& name)
{
    const std::wstring message = L"Save offset preset \"" + name + L"\"?";
    return MessageBoxW(hwnd, message.c_str(), L"Offset", MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1) == IDOK;
}

void updateActorJoints(AppWindowState& state, MappingActor& actor)
{
    std::lock_guard<std::mutex> lock(state.poseMutex);
    updateCalibratedMappingActorJoints(actor, state.latestPoseSnapshot, state.originEnabled, state.originOffset, state.originRotationDegrees);
}

void nudgeOffset(HWND hwnd, AppWindowState& state, MappingActor& actor, const MappingEditAxisButton& button)
{
    const int slot = state.selectedMappingOffsetSlot;
    const int axisCount = button.rotation ? 4 : 3;
    if (!button.valid || slot < 0 || slot >= kMappingSlotCount || button.axis < 0 || button.axis >= axisCount) {
        return;
    }
    const float delta = button.delta < 0.0f ? -selectedStepMeters(state) : selectedStepMeters(state);
    MappingTransform& offset = actor.calibration.trackerToTarget[static_cast<std::size_t>(slot)];
    if (button.rotation) {
        offset.rotation[static_cast<std::size_t>(button.axis)] += delta;
        offset.rotation = ovtr::normalizeQuaternion(offset.rotation);
    } else {
        Vec3& position = offset.position;
        if (button.axis == 0) {
            position.x += delta;
        } else if (button.axis == 1) {
            position.y += delta;
        } else {
            position.z += delta;
        }
    }

    updateActorJoints(state, actor);
    appendDebugLog(state, L"Mapping edit offset adjusted");
    refresh(hwnd, state);
}

void applyOffsets(HWND hwnd, AppWindowState& state, MappingActor& actor, const std::array<MappingTransform, kMappingSlotCount>& offsets)
{
    actor.calibration.trackerToTarget = offsets;
    updateActorJoints(state, actor);
    refresh(hwnd, state);
}

void saveSelectedOffsetPreset(HWND hwnd, AppWindowState& state, const MappingActor& actor)
{
    syncMappingOffsetPresetNameEditorText(state);
    const std::wstring name = trimWide(state.mappingEditOffsetPresetName);
    if (name.empty()) {
        MessageBoxW(hwnd, L"Enter an offset preset name first.", L"Offset", MB_OK | MB_ICONWARNING);
        return;
    }
    if (!confirmOffsetPresetSave(hwnd, name)) {
        appendDebugLog(state, L"Offset preset save canceled");
        return;
    }
    std::string error;
    if (!ensureMappingOffsetPresetDirectory(error)) {
        showOffsetError(hwnd, error);
        return;
    }
    const std::filesystem::path path = mappingOffsetPresetPathForName(name);
    std::error_code existsError;
    if (std::filesystem::exists(path, existsError)) {
        const int choice = MessageBoxW(
            hwnd,
            L"An offset preset with this name already exists. Overwrite it?",
            L"Offset",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
        );
        if (choice != IDYES) {
            return;
        }
    } else if (existsError) {
        showOffsetError(hwnd, "could not inspect offset preset: " + existsError.message());
        return;
    }
    if (!saveMappingOffsetPresetListToPath(name, actor.calibration.trackerToTarget, path, error)) {
        showOffsetError(hwnd, error);
        return;
    }
    state.mappingEditOffsetPresetDropdownOpen = false;
    appendDebugLog(state, L"Offset preset saved: " + path.filename().wstring());
    refresh(hwnd, state);
}

bool selectOffsetPresetDropdownOption(HWND hwnd, AppWindowState& state, const ProfilePanelLayout& panel, POINT point, MappingActor& actor)
{
    if (!state.mappingEditOffsetPresetDropdownOpen) {
        return false;
    }
    std::vector<MappingOffsetPresetFileEntry> presets;
    std::string error;
    if (!listSavedMappingOffsetPresets(presets, error)) {
        state.mappingEditOffsetPresetDropdownOpen = false;
        showOffsetError(hwnd, error);
        refresh(hwnd, state);
        return true;
    }
    const int option = mappingEditOffsetPresetDropdownOptionFromPoint(panel, static_cast<int>(presets.size()), point);
    if (option < 0) {
        return false;
    }
    if (option >= static_cast<int>(presets.size())) {
        state.mappingEditOffsetPresetDropdownOpen = false;
        refresh(hwnd, state);
        return true;
    }
    std::array<MappingTransform, kMappingSlotCount> offsets{};
    if (!loadMappingOffsetPresetListFromPath(presets[static_cast<std::size_t>(option)].path, offsets, error)) {
        state.mappingEditOffsetPresetDropdownOpen = false;
        showOffsetError(hwnd, error);
        refresh(hwnd, state);
        return true;
    }
    state.mappingEditOffsetPresetDropdownOpen = false;
    state.mappingEditOffsetPresetName = presets[static_cast<std::size_t>(option)].name;
    appendDebugLog(state, L"Offset preset loaded: " + presets[static_cast<std::size_t>(option)].name);
    applyOffsets(hwnd, state, actor, offsets);
    return true;
}

} // namespace

bool handleMappingEditPanelClick(HWND hwnd, AppWindowState& state, const int clientWidth, const int clientHeight, const POINT point)
{
    if (!state.editPanelVisible) {
        return false;
    }
    const ProfilePanelLayout panel = profilePanelLayoutForClient(&state, clientWidth, clientHeight);
    const MappingEditPanelLayout layout = mappingEditPanelLayoutForPanel(panel);
    if (!layout.valid) {
        return false;
    }
    MappingActor* actor = selectedActor(state);
    if (!actor || !actor->calibrated) {
        if (state.mappingEditStepDropdownOpen || state.mappingEditOffsetPresetDropdownOpen) {
            state.mappingEditStepDropdownOpen = false;
            state.mappingEditOffsetPresetDropdownOpen = false;
            refresh(hwnd, state);
        }
        return PtInRect(&panel.panelRect, point);
    }
    if (selectOffsetPresetDropdownOption(hwnd, state, panel, point, *actor)) {
        return true;
    }

    const MappingEditStepOptionLayout stepOption = mappingEditStepOptionAtPoint(panel, point);
    if (state.mappingEditStepDropdownOpen && stepOption.valid) {
        state.mappingEditOffsetStepMeters = stepOption.stepMeters;
        state.mappingEditStepDropdownOpen = false;
        state.mappingEditOffsetPresetDropdownOpen = false;
        refresh(hwnd, state);
        return true;
    }

    if (PtInRect(&layout.stepValueRect, point)) {
        closeMappingOffsetPresetNameEditor(hwnd, state);
        state.mappingEditStepDropdownOpen = !state.mappingEditStepDropdownOpen;
        state.mappingEditOffsetPresetDropdownOpen = false;
        refresh(hwnd, state);
        return true;
    }

    if (PtInRect(&layout.presetNameEditRect, point)) {
        state.mappingEditStepDropdownOpen = false;
        state.mappingEditOffsetPresetDropdownOpen = false;
        showMappingOffsetPresetNameEditor(hwnd, state);
        return true;
    }

    if (PtInRect(&layout.presetSaveButtonRect, point)) {
        state.mappingEditStepDropdownOpen = false;
        saveSelectedOffsetPreset(hwnd, state, *actor);
        return true;
    }

    if (PtInRect(&layout.presetValueRect, point)) {
        closeMappingOffsetPresetNameEditor(hwnd, state);
        state.mappingEditOffsetPresetDropdownOpen = !state.mappingEditOffsetPresetDropdownOpen;
        state.mappingEditStepDropdownOpen = false;
        refresh(hwnd, state);
        return true;
    }

    const bool closeDropdown = state.mappingEditStepDropdownOpen || state.mappingEditOffsetPresetDropdownOpen;
    state.mappingEditStepDropdownOpen = false;
    state.mappingEditOffsetPresetDropdownOpen = false;

    if (PtInRect(&layout.listScrollbarRect, point)) {
        const RECT thumb = mappingEditOffsetScrollbarThumbRect(layout, state.mappingEditOffsetScrollOffset);
        const int page = layout.visibleRowCount > 0 ? layout.visibleRowCount : 1;
        state.mappingEditOffsetScrollOffset += point.y < thumb.top ? -page : page;
        state.mappingEditOffsetScrollOffset = clampMappingEditOffsetScrollOffset(state.mappingEditOffsetScrollOffset, layout.visibleRowCount);
        refresh(hwnd, state);
        return true;
    }

    const MappingEditAxisButton button = mappingEditAxisButtonAtPoint(panel, point);
    if (button.valid) {
        nudgeOffset(hwnd, state, *actor, button);
        return true;
    }

    const MappingEditPanelRowLayout row = mappingEditRowAtPoint(panel, state.mappingEditOffsetScrollOffset, point);
    if (row.valid) {
        closeMappingOffsetPresetNameEditor(hwnd, state);
        if (state.selectedMappingOffsetSlot == row.slotIndex) {
            state.selectedMappingOffsetSlot = -1;
            state.mappingEditOffsetPresetName.clear();
        } else {
            state.selectedMappingOffsetSlot = row.slotIndex;
            if (state.mappingEditOffsetPresetName.empty()) {
                state.mappingEditOffsetPresetName = defaultOffsetPresetName(*actor);
            }
        }
        refresh(hwnd, state);
        return true;
    }
    if (closeDropdown) {
        refresh(hwnd, state);
    }
    return PtInRect(&panel.panelRect, point);
}

} // namespace ovtr::win32
