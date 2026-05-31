#include "platform/win32/MappingPanelDropdownPainter.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/MappingModel.h"
#include "platform/win32/MappingPresetStore.h"
#include "platform/win32/ProfileStore.h"
#include "platform/win32/Win32GdiResources.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ovtr::win32 {
namespace {

std::wstring deviceOptionText(const DeviceListRow& row)
{
    if (!row.customName.empty()) {
        return row.customName;
    }
    return row.serial;
}

void drawPopupFrame(HDC drawDc, const RECT& popupRect)
{
    UniqueBrush popupBrush(CreateSolidBrush(RGB(24, 28, 35)));
    UniquePen popupPen(CreatePen(PS_SOLID, 1, RGB(92, 126, 168)));
    SelectObjectGuard brushSelection(drawDc, popupBrush.get());
    SelectObjectGuard penSelection(drawDc, popupPen.get());
    Rectangle(drawDc, popupRect.left, popupRect.top, popupRect.right, popupRect.bottom);
}

} // namespace

std::wstring currentMappingText(const AppWindowState& state, const int slotIndex)
{
    if (!isMappingDeviceRow(slotIndex)) {
        return {};
    }

    const std::uint32_t runtimeIndex =
        state.mappingDeviceRuntimeIndices[static_cast<std::size_t>(slotIndex)];
    if (runtimeIndex == kNoSelectedRuntimeIndex) {
        return kMappingNoDeviceLabel;
    }

    for (const DeviceListRow& row : makeDeviceListRows(state)) {
        if (row.runtimeIndex == runtimeIndex) {
            return deviceOptionText(row);
        }
    }
    return L"(missing)";
}

void drawMappingDeviceDropdown(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const ProfilePanelLayout& layout,
    const std::vector<DeviceListRow>& deviceRows
)
{
    const MappingPanelRowLayout row =
        mappingRowLayoutForSlot(layout, state.mappingDropdownSlot, state.mappingScrollOffset);
    if (!row.valid) {
        return;
    }

    const int optionCount = static_cast<int>(deviceRows.size()) + 1;
    const RECT popupRect = mappingDropdownRectForRow(row, layout, optionCount);
    drawPopupFrame(drawDc, popupRect);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    const int rowHeight = row.rowRect.bottom - row.rowRect.top;
    for (int option = 0; option < optionCount && popupRect.top + option * rowHeight < popupRect.bottom; ++option) {
        RECT optionRect{popupRect.left + 6, popupRect.top + option * rowHeight,
                        popupRect.right - 6, popupRect.top + (option + 1) * rowHeight};
        const std::wstring text = option == 0
            ? std::wstring(kMappingNoDeviceLabel)
            : deviceOptionText(deviceRows[static_cast<std::size_t>(option - 1)]);
        DrawTextW(drawDc, text.c_str(), -1, &optionRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

void drawMappingProfileDropdown(
    HDC drawDc,
    HFONT font,
    const ProfilePanelLayout& layout,
    const MappingPanelControlsLayout& controls
)
{
    std::vector<ProfileFileEntry> profiles;
    std::string error;
    if (!listSavedProfiles(profiles, error)) {
        profiles.clear();
    }

    const int optionCount = static_cast<int>(profiles.size());
    const RECT popupRect = mappingProfileDropdownRectForControls(controls, layout, optionCount);
    drawPopupFrame(drawDc, popupRect);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    const int rowHeight = controls.profileBoxRect.bottom - controls.profileBoxRect.top;
    const int visibleRows = optionCount > 0 ? optionCount : 1;
    for (int option = 0; option < visibleRows && popupRect.top + option * rowHeight < popupRect.bottom; ++option) {
        RECT optionRect{popupRect.left + 6, popupRect.top + option * rowHeight,
                        popupRect.right - 6, popupRect.top + (option + 1) * rowHeight};
        const std::wstring text = optionCount > 0 ? profiles[static_cast<std::size_t>(option)].name
                                                  : std::wstring(L"No profiles");
        DrawTextW(drawDc, text.c_str(), -1, &optionRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

void drawMappingPresetDropdown(
    HDC drawDc,
    HFONT font,
    const ProfilePanelLayout& layout,
    const MappingPanelControlsLayout& controls
)
{
    std::vector<MappingPresetFileEntry> presets;
    std::string error;
    if (!listSavedMappingPresets(presets, error)) {
        presets.clear();
    }

    const int optionCount = static_cast<int>(presets.size());
    const RECT popupRect = mappingPresetDropdownRectForControls(controls, layout, optionCount);
    drawPopupFrame(drawDc, popupRect);

    SelectObjectGuard fontSelection(drawDc, font);
    SetTextColor(drawDc, RGB(225, 231, 240));
    const int rowHeight = controls.presetValueRect.bottom - controls.presetValueRect.top;
    const int visibleRows = optionCount > 0 ? optionCount : 1;
    for (int option = 0; option < visibleRows && popupRect.top + option * rowHeight < popupRect.bottom; ++option) {
        RECT optionRect{popupRect.left + 6, popupRect.top + option * rowHeight,
                        popupRect.right - 6, popupRect.top + (option + 1) * rowHeight};
        const std::wstring text = optionCount > 0 ? presets[static_cast<std::size_t>(option)].name
                                                  : std::wstring(L"No presets");
        DrawTextW(drawDc, text.c_str(), -1, &optionRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

} // namespace ovtr::win32
