#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ViewportColorDialogControls.h"
#include "platform/win32/ViewportSettingsModel.h"

#include <array>

namespace ovtr::win32 {

struct ViewportColorDialogState {
    ViewportSettings workingSettings;
    ViewportColorDialogControls controls;
    std::array<COLORREF, 16> customColors{};
    bool accepted = false;
    bool done = false;
};

void updateViewportColorDialogControls(ViewportColorDialogState& dialog);
void chooseViewportDialogColor(HWND hwnd, ViewportColorDialogState& dialog, int colorIndex);
void resetViewportDialogColors(ViewportColorDialogState& dialog);
void finishViewportColorDialog(HWND hwnd, ViewportColorDialogState& dialog, bool accepted);

} // namespace ovtr::win32
