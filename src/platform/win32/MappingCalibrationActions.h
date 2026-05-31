#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

void calibrateSelectedMappingActor(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
