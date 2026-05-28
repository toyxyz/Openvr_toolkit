#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "data/SessionTypes.h"

#include <cstdint>
#include <string>

namespace ovtr::win32 {

struct AppWindowState;

bool addMarkerFromSelectedDevice(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& device);
bool renameSelectedMarker(HWND hwnd, AppWindowState& state);
bool deleteSelectedMarker(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
