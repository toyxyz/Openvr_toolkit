#pragma once

#include "platform/win32/ViewportColorDialogSession.h"

namespace ovtr::win32 {

COLORREF colorRefFromRgb(RgbColor color);
RgbColor rgbFromColorRef(COLORREF color);
bool readViewportColorDialogControls(HWND hwnd, ViewportColorDialogState& dialog);
bool drawViewportColorSwatch(const ViewportColorDialogState& dialog, const DRAWITEMSTRUCT& item);

} // namespace ovtr::win32
