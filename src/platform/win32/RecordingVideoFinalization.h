#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <string>

namespace ovtr::win32 {

struct AppWindowState;

bool requestRecordingVideoFinalization(
    HWND hwnd,
    AppWindowState& state,
    bool trackerStopSucceeded,
    std::string trackerStopError,
    std::uint64_t stoppedFrameCount,
    std::uint64_t stoppedDroppedFrames,
    std::string& outError
);

void pollRecordingVideoFinalization(HWND hwnd, AppWindowState& state);

} // namespace ovtr::win32
