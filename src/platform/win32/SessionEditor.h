#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void closeSessionEditor(HWND hwnd, AppWindowState& state);
void showSessionEditor(HWND hwnd, AppWindowState& state);
void updateSessionEditorLayout(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
