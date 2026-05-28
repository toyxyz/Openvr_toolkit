#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/ConfigTypes.h"

namespace ovtr::win32 {

struct AppWindowState;

void updateDelayedRecordingStart(HWND hwnd, AppWindowState& state);
void toggleRecording(HWND hwnd);
void exportCurrentSession(HWND hwnd, ExportFormat format);

} // namespace ovtr::win32
