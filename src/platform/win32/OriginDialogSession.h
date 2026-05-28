#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/OriginDialogControls.h"
#include "platform/win32/OriginDialogModel.h"

namespace ovtr::win32 {

struct AppWindowState;

struct OriginDialogState {
    HWND parent = nullptr;
    AppWindowState* appState = nullptr;
    OriginDialogValues originalValues;
    OriginDialogValues workingValues;
    OriginDialogControls controls;
    bool updatingControls = false;
    bool accepted = false;
    bool done = false;
};

void updateOriginDialogControls(OriginDialogState& dialog);
void previewOriginDialogFromControls(HWND hwnd, OriginDialogState& dialog);
void restoreOriginDialogSnapshot(HWND hwnd, OriginDialogState& dialog);
void finishOriginDialog(HWND hwnd, OriginDialogState& dialog, bool accepted);

} // namespace ovtr::win32
