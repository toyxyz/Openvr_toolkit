#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

LRESULT CALLBACK viewportProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

} // namespace ovtr::win32
