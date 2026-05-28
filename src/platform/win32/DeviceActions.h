#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr {
struct DeviceDescriptor;
}

namespace ovtr::win32 {

struct AppWindowState;

bool setDeviceCustomName(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& device);

} // namespace ovtr::win32
