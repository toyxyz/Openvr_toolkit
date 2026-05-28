#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

bool handleMainWindowCreate(HWND hwnd, LPARAM lparam);
void handleMainWindowDestroy(HWND hwnd, UINT_PTR timerId);

} // namespace ovtr::win32
