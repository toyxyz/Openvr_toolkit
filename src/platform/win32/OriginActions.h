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

bool confirmSetOrigin(HWND hwnd, AppWindowState& state, const ovtr::DeviceDescriptor& selected);
void clearMissingDeviceSelectionWithLog(AppWindowState& state);
void toggleListDeviceSelection(AppWindowState& state, const ovtr::DeviceDescriptor& device);
void ensureOriginSelectionForUi(AppWindowState& state);
void selectNextOriginDeviceWithLog(AppWindowState& state);
bool setOriginFromDevice(AppWindowState& state, const ovtr::DeviceDescriptor& selected);
void clearOrigin(AppWindowState& state);

} // namespace ovtr::win32
