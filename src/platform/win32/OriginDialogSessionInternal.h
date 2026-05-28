#pragma once

#include "platform/win32/OriginDialogSession.h"

namespace ovtr::win32 {

bool readOriginDialogControls(HWND hwnd, OriginDialogState& dialog, bool showWarning);
void renderOriginDialogPreview(HWND hwnd, OriginDialogState& dialog);

} // namespace ovtr::win32
