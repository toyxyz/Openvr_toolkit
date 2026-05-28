#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

void refreshStatus(HWND hwnd, bool forceDeviceEnumeration = false);

} // namespace ovtr::win32
