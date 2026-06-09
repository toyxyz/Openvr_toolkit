#include "platform/win32/SideMenuVisibility.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

SideMenuVisibilitySnapshot captureSideMenuVisibility(const AppWindowState& state) noexcept
{
    return {
        state.devicePanelVisible,
        state.sessionPanelVisible,
        state.streamingPanelVisible,
        state.profilePanelVisible,
        state.mappingPanelVisible,
        state.editPanelVisible
    };
}

bool anySideMenuVisible(const AppWindowState& state) noexcept
{
    const SideMenuVisibilitySnapshot snapshot = captureSideMenuVisibility(state);
    return snapshot.devicePanelVisible ||
        snapshot.sessionPanelVisible ||
        snapshot.streamingPanelVisible ||
        snapshot.profilePanelVisible ||
        snapshot.mappingPanelVisible ||
        snapshot.editPanelVisible;
}

void hideSideMenusForShortcut(AppWindowState& state) noexcept
{
    state.sideMenuVisibilityBeforeHide = captureSideMenuVisibility(state);
    state.sideMenusHiddenByShortcut = true;
    state.devicePanelVisible = false;
    state.sessionPanelVisible = false;
    state.streamingPanelVisible = false;
    state.profilePanelVisible = false;
    state.mappingPanelVisible = false;
    state.editPanelVisible = false;
    state.splitterDragging = false;
    state.profileSplitterDragging = false;
}

void restoreSideMenusForShortcut(AppWindowState& state) noexcept
{
    const SideMenuVisibilitySnapshot snapshot = state.sideMenuVisibilityBeforeHide;
    state.devicePanelVisible = snapshot.devicePanelVisible;
    state.sessionPanelVisible = snapshot.sessionPanelVisible;
    state.streamingPanelVisible = snapshot.streamingPanelVisible;
    state.profilePanelVisible = snapshot.profilePanelVisible;
    state.mappingPanelVisible = snapshot.mappingPanelVisible;
    state.editPanelVisible = snapshot.editPanelVisible;
    state.sideMenusHiddenByShortcut = false;
    state.splitterDragging = false;
    state.profileSplitterDragging = false;
}

void toggleSideMenusForShortcut(AppWindowState& state) noexcept
{
    if (state.sideMenusHiddenByShortcut) {
        restoreSideMenusForShortcut(state);
        return;
    }
    hideSideMenusForShortcut(state);
}

} // namespace ovtr::win32
