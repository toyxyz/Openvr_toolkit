#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ovtr::win32 {

struct AppWindowState;
struct VideoCameraInfo;

void requestVideoCameraRefresh(AppWindowState& state);
void pollVideoCameraTasks(HWND hwnd, AppWindowState& state);
bool requestVideoCameraSwitch(AppWindowState& state, const VideoCameraInfo& camera);
bool requestVideoCameraDisable(AppWindowState& state);

} // namespace ovtr::win32
