#pragma once

namespace ovtr::win32 {

struct SideMenuVisibilitySnapshot {
    bool devicePanelVisible = true;
    bool sessionPanelVisible = false;
    bool streamingPanelVisible = false;
    bool profilePanelVisible = false;
    bool mappingPanelVisible = false;
    bool editPanelVisible = false;
};

struct AppSideMenuState {
    bool sideMenusHiddenByShortcut = false;
    SideMenuVisibilitySnapshot sideMenuVisibilityBeforeHide;
};

} // namespace ovtr::win32
