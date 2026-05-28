#include "platform/win32/ViewportColorDialogSessionInternal.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"

#include <commdlg.h>

namespace ovtr::win32 {

void chooseViewportDialogColor(HWND hwnd, ViewportColorDialogState& dialog, const int colorIndex)
{
    if (!readViewportColorDialogControls(hwnd, dialog)) {
        return;
    }

    CHOOSECOLORW chooser{};
    chooser.lStructSize = sizeof(chooser);
    chooser.hwndOwner = hwnd;
    chooser.rgbResult = colorRefFromRgb(viewportColorSlot(dialog.workingSettings, colorIndex));
    chooser.lpCustColors = dialog.customColors.data();
    chooser.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColorW(&chooser)) {
        setViewportColorSlot(dialog.workingSettings, colorIndex, rgbFromColorRef(chooser.rgbResult));
        updateViewportColorDialogControls(dialog);
    }
}

void resetViewportDialogColors(ViewportColorDialogState& dialog)
{
    float currentOutlineMultiplier = dialog.workingSettings.outlineMultiplier;
    if (readFloatEdit(dialog.controls.outlineEdit, currentOutlineMultiplier)) {
        dialog.workingSettings.outlineMultiplier = currentOutlineMultiplier;
    }
    float currentGridSize = dialog.workingSettings.gridSize;
    if (readFiniteFloatEdit(dialog.controls.gridSizeEdit, currentGridSize)) {
        dialog.workingSettings.gridSize = currentGridSize;
    }
    float currentGridDensity = dialog.workingSettings.gridCellDensity;
    if (readFiniteFloatEdit(dialog.controls.gridDensityEdit, currentGridDensity)) {
        dialog.workingSettings.gridCellDensity = currentGridDensity;
    }
    float currentMarkerSize = dialog.workingSettings.markerSize;
    if (readFiniteFloatEdit(dialog.controls.markerSizeEdit, currentMarkerSize)) {
        dialog.workingSettings.markerSize = currentMarkerSize;
    }

    dialog.workingSettings = viewportSettingsWithDefaultColors(dialog.workingSettings);
    dialog.workingSettings = viewportSettingsWithDefaultGrid(dialog.workingSettings);
    dialog.workingSettings = viewportSettingsWithDefaultMarker(dialog.workingSettings);
    updateViewportColorDialogControls(dialog);
}

void finishViewportColorDialog(HWND hwnd, ViewportColorDialogState& dialog, const bool accepted)
{
    if (accepted && !readViewportColorDialogControls(hwnd, dialog)) {
        return;
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

} // namespace ovtr::win32
