#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;

bool handleMappingEditPanelClick(HWND hwnd, AppWindowState& state, int clientWidth, int clientHeight, POINT point);

} // namespace ovtr::win32
