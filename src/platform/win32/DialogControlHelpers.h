#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

inline void applyControlFont(HWND control, HFONT font) noexcept
{
    if (control) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

inline void focusAndSelectAllText(HWND editControl) noexcept
{
    if (editControl) {
        SetFocus(editControl);
        SendMessageW(editControl, EM_SETSEL, 0, -1);
    }
}

} // namespace ovtr::win32
