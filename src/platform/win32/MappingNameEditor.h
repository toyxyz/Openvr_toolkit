#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void closeMappingNameEditor(HWND hwnd, AppWindowState& state);
void showMappingNameEditor(HWND hwnd, AppWindowState& state);
void updateMappingNameEditorLayout(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
