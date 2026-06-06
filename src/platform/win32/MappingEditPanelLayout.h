#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/LayoutTypes.h"

#include <array>
#include <vector>

namespace ovtr::win32 {

struct MappingEditPanelRowLayout {
    RECT rowRect{0, 0, 0, 0};
    RECT labelRect{0, 0, 0, 0};
    int slotIndex = -1;
    bool valid = false;
};

struct MappingEditAxisButton {
    RECT rect{0, 0, 0, 0};
    int axis = -1;
    float delta = 0.0f;
    bool rotation = false;
    bool valid = false;
};

struct MappingEditStepOptionLayout {
    RECT rowRect{0, 0, 0, 0};
    float stepMeters = 0.0f;
    bool valid = false;
};

struct MappingEditPanelLayout {
    RECT actorNameRect{0, 0, 0, 0};
    RECT listRect{0, 0, 0, 0};
    RECT listContentRect{0, 0, 0, 0};
    RECT listScrollbarRect{0, 0, 0, 0};
    RECT valuesRect{0, 0, 0, 0};
    RECT stepValueRect{0, 0, 0, 0};
    RECT presetBoxRect{0, 0, 0, 0};
    RECT presetNameLabelRect{0, 0, 0, 0};
    RECT presetNameEditRect{0, 0, 0, 0};
    RECT presetSaveButtonRect{0, 0, 0, 0};
    RECT presetValueRect{0, 0, 0, 0};
    int rowHeight = 0;
    int visibleRowCount = 0;
    bool valid = false;
};

inline constexpr std::array<float, 3> kMappingEditOffsetStepOptionsMeters{0.001f, 0.01f, 0.1f};

MappingEditPanelLayout mappingEditPanelLayoutForPanel(const ProfilePanelLayout& panel) noexcept;
int maxMappingEditOffsetScrollOffset(int visibleRowCount) noexcept;
int clampMappingEditOffsetScrollOffset(int scrollOffset, int visibleRowCount) noexcept;
RECT mappingEditOffsetScrollbarThumbRect(const MappingEditPanelLayout& layout, int scrollOffset) noexcept;
std::vector<MappingEditPanelRowLayout> mappingEditRowsForPanel(const ProfilePanelLayout& panel, int scrollOffset);
MappingEditPanelRowLayout mappingEditRowAtPoint(const ProfilePanelLayout& panel, int scrollOffset, POINT point);
std::vector<MappingEditAxisButton> mappingEditAxisButtonsForPanel(const ProfilePanelLayout& panel);
MappingEditAxisButton mappingEditAxisButtonAtPoint(const ProfilePanelLayout& panel, POINT point);
std::vector<MappingEditStepOptionLayout> mappingEditStepOptionsForPanel(const ProfilePanelLayout& panel);
MappingEditStepOptionLayout mappingEditStepOptionAtPoint(const ProfilePanelLayout& panel, POINT point);
RECT mappingEditOffsetPresetDropdownRectForPanel(const ProfilePanelLayout& panel, int optionCount) noexcept;
int mappingEditOffsetPresetDropdownOptionFromPoint(
    const ProfilePanelLayout& panel,
    int optionCount,
    POINT point
) noexcept;

} // namespace ovtr::win32
