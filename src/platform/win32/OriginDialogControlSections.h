#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/OriginDialogControls.h"

namespace ovtr::win32 {

HWND createOriginDialogEnabledCheckbox(HWND hwnd, HFONT font);
void createOriginDialogAxisHeaders(HWND hwnd, HFONT font);
void createOriginDialogValueRows(HWND hwnd, HFONT font, OriginDialogControls& controls);
void createOriginDialogActionButtons(HWND hwnd, HFONT font);
void focusFirstOriginDialogEdit(const OriginDialogControls& controls);

} // namespace ovtr::win32
