#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppProfileState.h"
#include "platform/win32/LayoutTypes.h"

#include <vector>

namespace ovtr::win32 {

struct ProfilePanelFieldLayout {
    RECT rowRect{0, 0, 0, 0};
    RECT labelRect{0, 0, 0, 0};
    RECT valueRect{0, 0, 0, 0};
    ProfileEditTarget target;
    bool valid = false;
};

struct ProfilePanelControlsLayout {
    RECT tableRect{0, 0, 0, 0};
    RECT previewButtonRect{0, 0, 0, 0};
    RECT saveButtonRect{0, 0, 0, 0};
    RECT loadButtonRect{0, 0, 0, 0};
    int rowHeight = 0;
    int visibleRowCount = 0;
    bool valid = false;
};

ProfilePanelControlsLayout profileControlsLayoutForPanel(const ProfilePanelLayout& panelLayout) noexcept;
int maxProfileScrollOffset(int visibleRowCount) noexcept;
int clampProfileScrollOffset(int scrollOffset, int visibleRowCount) noexcept;
std::vector<ProfilePanelFieldLayout> profileFieldLayoutsForPanel(
    const ProfilePanelLayout& panelLayout,
    int scrollOffset = 0
);
ProfilePanelFieldLayout profileFieldLayoutAtPoint(
    const ProfilePanelLayout& panelLayout,
    POINT point,
    int scrollOffset = 0
);

} // namespace ovtr::win32
