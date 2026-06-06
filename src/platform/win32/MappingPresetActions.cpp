#include "platform/win32/MappingPresetActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingPresetStore.h"
#include "platform/win32/Win32String.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
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

bool hasExistingFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error);
}

std::wstring serialForRuntimeIndex(const std::vector<DeviceListRow>& rows, const std::uint32_t runtimeIndex)
{
    for (const DeviceListRow& row : rows) {
        if (row.runtimeIndex == runtimeIndex) {
            return row.serial;
        }
    }
    return {};
}

void applyMappingPreset(AppWindowState& state, const MappingPreset& preset)
{
    if (preset.hasProfile) {
        state.profile = preset.profile;
    }
    state.mappingSkeletonColor = preset.skeletonColor;
    state.mappingSkeletonColorCustomized = true;

    const std::vector<DeviceListRow> rows = makeDeviceListRows(state);
    state.mappingDeviceRuntimeIndices = defaultMappingDeviceRuntimeIndices();
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const std::wstring& serial = preset.deviceSerials[static_cast<std::size_t>(slot)];
        for (const DeviceListRow& row : rows) {
            if (!serial.empty() && row.serial == serial) {
                state.mappingDeviceRuntimeIndices[static_cast<std::size_t>(slot)] = row.runtimeIndex;
                break;
            }
        }
    }
    syncSelectedMappingActorFromControls(state);
}

} // namespace

void saveCurrentMappingPreset(HWND hwnd, AppWindowState& state)
{
    std::wstring stem = sanitizedMappingPresetFileStem(state.mappingPresetName);
    if (stem.empty()) {
        MessageBoxW(hwnd, L"Mapping name cannot be empty.", L"Mapping", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string error;
    if (!ensureMappingPresetDirectory(error)) {
        showMappingError(hwnd, error);
        return;
    }

    MappingPreset preset;
    preset.name = stem;
    preset.hasProfile = true;
    preset.profile = state.profile;
    preset.skeletonColor = state.mappingSkeletonColor;
    const std::vector<DeviceListRow> rows = makeDeviceListRows(state);
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const std::uint32_t runtimeIndex = state.mappingDeviceRuntimeIndices[static_cast<std::size_t>(slot)];
        preset.deviceSerials[static_cast<std::size_t>(slot)] = serialForRuntimeIndex(rows, runtimeIndex);
    }

    const std::filesystem::path path = mappingPresetPathForName(stem);
    if (hasExistingFile(path)) {
        const int choice = MessageBoxW(
            hwnd,
            L"A mapping preset with this name already exists. Overwrite it?",
            L"Mapping",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
        );
        if (choice != IDYES) {
            appendDebugLog(state, L"Mapping preset save canceled");
            return;
        }
    }
    if (!saveMappingPresetToPath(preset, path, error)) {
        showMappingError(hwnd, error);
        return;
    }
    state.mappingPresetName = stem;
    state.mappingPresetDropdownOpen = false;
    appendDebugLog(state, L"Mapping preset saved: " + path.filename().wstring());
    refreshMappingUi(hwnd);
}

bool selectMappingPresetDropdownOption(
    HWND hwnd,
    AppWindowState& state,
    const ProfilePanelLayout& panelLayout,
    const MappingPanelControlsLayout& controls,
    const POINT point
)
{
    if (!state.mappingPresetDropdownOpen) {
        return false;
    }

    std::vector<MappingPresetFileEntry> presets;
    std::string error;
    if (!listSavedMappingPresets(presets, error)) {
        state.mappingPresetDropdownOpen = false;
        showMappingError(hwnd, error);
        refreshMappingUi(hwnd);
        return true;
    }

    const int option = mappingPresetDropdownOptionFromPoint(
        controls,
        panelLayout,
        static_cast<int>(presets.size()),
        point
    );
    if (option < 0) {
        return false;
    }
    if (option >= static_cast<int>(presets.size())) {
        state.mappingPresetDropdownOpen = false;
        refreshMappingUi(hwnd);
        return true;
    }

    MappingPreset preset;
    if (!loadMappingPresetFromPath(presets[static_cast<std::size_t>(option)].path, preset, error)) {
        state.mappingPresetDropdownOpen = false;
        showMappingError(hwnd, error);
        refreshMappingUi(hwnd);
        return true;
    }
    applyMappingPreset(state, preset);
    state.mappingPresetName = preset.name;
    state.mappingPresetDropdownOpen = false;
    appendDebugLog(state, L"Mapping preset loaded: " + presets[static_cast<std::size_t>(option)].name);
    refreshMappingViewport(hwnd, state);
    return true;
}

} // namespace ovtr::win32
