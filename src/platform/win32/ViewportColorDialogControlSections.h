#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ViewportColorDialogControls.h"

namespace ovtr::win32 {

void createViewportColorHeaderControls(HWND hwnd, HFONT font);
void createViewportColorRows(HWND hwnd, HFONT font, ViewportColorDialogControls& controls);
void createViewportColorFooterControls(HWND hwnd, HFONT font, ViewportColorDialogControls& controls);

} // namespace ovtr::win32
