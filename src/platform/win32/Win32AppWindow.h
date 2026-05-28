#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

bool registerBootstrapWindowClasses(HINSTANCE instance);
HWND createMainWindow(HINSTANCE instance);
int runMessageAndFrameLoop(HWND hwnd);

} // namespace ovtr::win32
