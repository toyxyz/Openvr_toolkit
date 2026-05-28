#include "platform/win32/ViewportColorDialogSessionInternal.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"

#include <cstddef>

namespace ovtr::win32 {
namespace {

bool readColorEditControls(const ViewportColorEditControls& controls, RgbColor& color)
{
    return readIntegerEdit(controls.red, color.r) &&
        readIntegerEdit(controls.green, color.g) &&
        readIntegerEdit(controls.blue, color.b);
}

RgbColor swatchColorForSlot(const ViewportColorDialogState& dialog, const int index)
{
    RgbColor color = viewportColorSlot(dialog.workingSettings, index);
    const ViewportColorEditControls& controls = dialog.controls.colors[static_cast<std::size_t>(index)];
    if (readColorEditControls(controls, color)) {
        return clampRgbColor(color);
    }
    return color;
}

void invalidateColorSwatches(const ViewportColorDialogControls& controls) noexcept
{
    for (const ViewportColorEditControls& colorControls : controls.colors) {
        if (colorControls.swatch) {
            InvalidateRect(colorControls.swatch, nullptr, TRUE);
        }
    }
}

} // namespace

bool readViewportColorDialogControls(HWND hwnd, ViewportColorDialogState& dialog)
{
    for (int i = 0; i < kViewportColorSlotCount; ++i) {
        RgbColor color{};
        const ViewportColorEditControls& controls = dialog.controls.colors[static_cast<std::size_t>(i)];
        if (!readColorEditControls(controls, color)) {
            MessageBoxW(hwnd, L"RGB values must be numbers from 0 to 255.", L"Appearance", MB_OK | MB_ICONWARNING);
            return false;
        }
        setViewportColorSlot(dialog.workingSettings, i, color);
    }

    float outlineMultiplier = 1.0f;
    if (!readFloatEdit(dialog.controls.outlineEdit, outlineMultiplier)) {
        MessageBoxW(hwnd, L"Outline thickness must be a number from 0.0 to 10.0.", L"Appearance", MB_OK | MB_ICONWARNING);
        return false;
    }
    dialog.workingSettings.outlineMultiplier = outlineMultiplier;

    float gridSize = 5.0f;
    if (!readFiniteFloatEdit(dialog.controls.gridSizeEdit, gridSize)) {
        MessageBoxW(hwnd, L"Grid size must be a number from 1.0 to 50.0.", L"Appearance", MB_OK | MB_ICONWARNING);
        return false;
    }
    dialog.workingSettings.gridSize = gridSize;

    float gridCellDensity = 2.0f;
    if (!readFiniteFloatEdit(dialog.controls.gridDensityEdit, gridCellDensity)) {
        MessageBoxW(hwnd, L"Grid cell density must be a number from 0.25 to 10.0.", L"Appearance", MB_OK | MB_ICONWARNING);
        return false;
    }
    dialog.workingSettings.gridCellDensity = gridCellDensity;
    return true;
}

void updateViewportColorDialogControls(ViewportColorDialogState& dialog)
{
    for (int i = 0; i < kViewportColorSlotCount; ++i) {
        const RgbColor color = viewportColorSlot(dialog.workingSettings, i);
        const ViewportColorEditControls& controls = dialog.controls.colors[static_cast<std::size_t>(i)];
        setEditText(controls.red, formatIntegerText(color.r));
        setEditText(controls.green, formatIntegerText(color.g));
        setEditText(controls.blue, formatIntegerText(color.b));
    }
    setEditText(dialog.controls.outlineEdit, formatFloatText(dialog.workingSettings.outlineMultiplier));
    setEditText(dialog.controls.gridSizeEdit, formatFloatText(dialog.workingSettings.gridSize));
    setEditText(dialog.controls.gridDensityEdit, formatFloatText(dialog.workingSettings.gridCellDensity));
    invalidateColorSwatches(dialog.controls);
}

bool drawViewportColorSwatch(const ViewportColorDialogState& dialog, const DRAWITEMSTRUCT& item)
{
    if (item.CtlID < kViewportColorSwatchBaseControlId ||
        item.CtlID >= kViewportColorSwatchBaseControlId + kViewportColorSlotCount) {
        return false;
    }

    const int index = static_cast<int>(item.CtlID - kViewportColorSwatchBaseControlId);
    const RgbColor color = swatchColorForSlot(dialog, index);
    HBRUSH fillBrush = CreateSolidBrush(colorRefFromRgb(color));
    if (fillBrush) {
        FillRect(item.hDC, &item.rcItem, fillBrush);
        DeleteObject(fillBrush);
    }
    FrameRect(item.hDC, &item.rcItem, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    return true;
}

} // namespace ovtr::win32
