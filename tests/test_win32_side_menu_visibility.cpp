#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppState.h"
#include "platform/win32/SideMenuVisibility.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::test {

void testWin32SideMenuVisibility()
{
    ovtr::win32::AppWindowState state;
    state.devicePanelVisible = false;
    state.sessionPanelVisible = true;
    state.streamingPanelVisible = false;
    state.profilePanelVisible = false;
    state.mappingPanelVisible = true;
    state.editPanelVisible = false;
    state.leftPanelWidth = 500;
    state.profilePanelWidth = 400;

    require(ovtr::win32::anySideMenuVisible(state), "side menu starts visible");
    ovtr::win32::hideSideMenusForShortcut(state);
    require(state.sideMenusHiddenByShortcut, "side menu hidden flag set");
    require(!state.devicePanelVisible, "device panel hidden by shortcut");
    require(!state.sessionPanelVisible, "session panel hidden by shortcut");
    require(!state.mappingPanelVisible, "mapping panel hidden by shortcut");
    require(ovtr::win32::leftPanelWidthForClient(&state, 1200) == 0, "hidden left rail width is zero");
    require(ovtr::win32::rightProfileAreaWidthForClient(&state, 1200) == 0, "hidden right rail width is zero");

    ovtr::win32::restoreSideMenusForShortcut(state);
    require(!state.sideMenusHiddenByShortcut, "side menu hidden flag cleared");
    require(!state.devicePanelVisible, "device panel restore keeps prior state");
    require(state.sessionPanelVisible, "session panel restores");
    require(state.mappingPanelVisible, "mapping panel restores");
    require(!state.profilePanelVisible, "profile panel restore keeps prior state");

    ovtr::win32::toggleSideMenusForShortcut(state);
    require(state.sideMenusHiddenByShortcut, "tab toggle hides side menus");
    ovtr::win32::toggleSideMenusForShortcut(state);
    require(state.sessionPanelVisible && state.mappingPanelVisible, "tab toggle restores snapshot");
}

} // namespace ovtr::test
