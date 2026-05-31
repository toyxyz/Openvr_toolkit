#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

#include <vector>

namespace ovtr::win32 {

struct MappingPanelRowLayout {
    RECT rowRect{0, 0, 0, 0};
    RECT labelRect{0, 0, 0, 0};
    RECT valueRect{0, 0, 0, 0};
    int slotIndex = -1;
    bool valid = false;
};

struct MappingPanelControlsLayout {
    RECT profileBoxRect{0, 0, 0, 0};
    RECT profileLabelRect{0, 0, 0, 0};
    RECT profileValueRect{0, 0, 0, 0};
    RECT nameBoxRect{0, 0, 0, 0};
    RECT nameLabelRect{0, 0, 0, 0};
    RECT nameValueRect{0, 0, 0, 0};
    RECT tableRect{0, 0, 0, 0};
    RECT presetSaveButtonRect{0, 0, 0, 0};
    RECT presetValueRect{0, 0, 0, 0};
    RECT addActorButtonRect{0, 0, 0, 0};
    RECT actorListRect{0, 0, 0, 0};
    RECT calibrateButtonRect{0, 0, 0, 0};
    RECT filterBoxRect{0, 0, 0, 0};
    RECT filterArmLabelRect{0, 0, 0, 0};
    RECT filterArmValueRect{0, 0, 0, 0};
    RECT filterLegLabelRect{0, 0, 0, 0};
    RECT filterLegValueRect{0, 0, 0, 0};
    int rowHeight = 0;
    int visibleRowCount = 0;
    bool valid = false;
};

MappingPanelControlsLayout mappingControlsLayoutForPanel(const ProfilePanelLayout& panelLayout) noexcept;
int maxMappingScrollOffset(int visibleRowCount) noexcept;
int clampMappingScrollOffset(int scrollOffset, int visibleRowCount) noexcept;
std::vector<MappingPanelRowLayout> mappingRowLayoutsForPanel(
    const ProfilePanelLayout& panelLayout,
    int scrollOffset = 0
);
MappingPanelRowLayout mappingRowLayoutAtPoint(
    const ProfilePanelLayout& panelLayout,
    POINT point,
    int scrollOffset = 0
);
MappingPanelRowLayout mappingRowLayoutForSlot(
    const ProfilePanelLayout& panelLayout,
    int slotIndex,
    int scrollOffset = 0
);
RECT mappingDropdownRectForRow(
    const MappingPanelRowLayout& row,
    const ProfilePanelLayout& panelLayout,
    int optionCount
) noexcept;
int mappingDropdownOptionFromPoint(
    const MappingPanelRowLayout& row,
    const ProfilePanelLayout& panelLayout,
    int optionCount,
    POINT point
) noexcept;
RECT mappingProfileDropdownRectForControls(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    int optionCount
) noexcept;
int mappingProfileDropdownOptionFromPoint(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    int optionCount,
    POINT point
) noexcept;
RECT mappingPresetDropdownRectForControls(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    int optionCount
) noexcept;
int mappingPresetDropdownOptionFromPoint(
    const MappingPanelControlsLayout& controls,
    const ProfilePanelLayout& panelLayout,
    int optionCount,
    POINT point
) noexcept;

} // namespace ovtr::win32
