#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/MappingPanelLayout.h"

#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppWindowState;
struct DeviceListRow;

std::wstring currentMappingText(const AppWindowState& state, int slotIndex);
void drawMappingDeviceDropdown(
    HDC drawDc,
    HFONT font,
    const AppWindowState& state,
    const ProfilePanelLayout& layout,
    const std::vector<DeviceListRow>& deviceRows
);
void drawMappingProfileDropdown(
    HDC drawDc,
    HFONT font,
    const ProfilePanelLayout& layout,
    const MappingPanelControlsLayout& controls
);
void drawMappingPresetDropdown(
    HDC drawDc,
    HFONT font,
    const ProfilePanelLayout& layout,
    const MappingPanelControlsLayout& controls
);

} // namespace ovtr::win32
