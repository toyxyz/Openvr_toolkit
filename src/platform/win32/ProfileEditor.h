#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;
struct ProfileEditTarget;

void closeProfileEditor(HWND hwnd, AppWindowState& state);
void showProfileEditor(HWND hwnd, AppWindowState& state, ProfileEditTarget target);
void updateProfileEditorLayout(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
