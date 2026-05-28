#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

WNDPROC installOriginEditorSubclass(HWND editWindow);

} // namespace ovtr::win32
