#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

void closeStreamingPortEditor(HWND hwnd, AppWindowState& state);
void closeStreamingHostEditor(HWND hwnd, AppWindowState& state);
void closeStreamingArmSpacingEditor(HWND hwnd, AppWindowState& state);
void closeStreamingLegSpacingEditor(HWND hwnd, AppWindowState& state);
void closeStreamingSpacingEditors(HWND hwnd, AppWindowState& state);
void showStreamingPortEditor(HWND hwnd, AppWindowState& state);
void showStreamingHostEditor(HWND hwnd, AppWindowState& state);
void showStreamingArmSpacingEditor(HWND hwnd, AppWindowState& state);
void showStreamingLegSpacingEditor(HWND hwnd, AppWindowState& state);
void updateStreamingPortEditorLayout(HWND hwnd, AppWindowState& state);
void updateStreamingHostEditorLayout(HWND hwnd, AppWindowState& state);
void updateStreamingArmSpacingEditorLayout(HWND hwnd, AppWindowState& state);
void updateStreamingLegSpacingEditorLayout(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
