#include "platform/win32/WindowKeyboardSections.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/SideMenuVisibility.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {

bool handleSideMenuVisibilityKeyDown(HWND hwnd, AppWindowState* state, const WPARAM wparam)
{
    if (wparam != VK_TAB) {
        return false;
    }
    if (!state) {
        return true;
    }
    const bool restoring = state->sideMenusHiddenByShortcut;
    toggleSideMenusForShortcut(*state);
    appendDebugLog(*state, restoring ? L"Side menus restored" : L"Side menus hidden");
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    return true;
}

} // namespace ovtr::win32
