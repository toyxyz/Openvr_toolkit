#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void closeMappingOffsetPresetNameEditor(HWND hwnd, AppWindowState& state);
void showMappingOffsetPresetNameEditor(HWND hwnd, AppWindowState& state);
void updateMappingOffsetPresetNameEditorLayout(HWND hwnd, AppWindowState& state);
void syncMappingOffsetPresetNameEditorText(AppWindowState& state);

} // namespace ovtr::win32
