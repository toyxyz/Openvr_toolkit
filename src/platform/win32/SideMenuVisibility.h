#pragma once

#include "platform/win32/AppSideMenuState.h"

namespace ovtr::win32 {

struct AppWindowState;

SideMenuVisibilitySnapshot captureSideMenuVisibility(const AppWindowState& state) noexcept;
bool anySideMenuVisible(const AppWindowState& state) noexcept;
void hideSideMenusForShortcut(AppWindowState& state) noexcept;
void restoreSideMenusForShortcut(AppWindowState& state) noexcept;
void toggleSideMenusForShortcut(AppWindowState& state) noexcept;

} // namespace ovtr::win32
