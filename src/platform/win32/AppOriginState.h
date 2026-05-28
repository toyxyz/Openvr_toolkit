#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/AppStateConstants.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <string>

namespace ovtr::win32 {

struct AppOriginState {
    mutable std::mutex originMutex;
    bool originEnabled = false;
    std::array<float, 3> originOffset{0.0f, 0.0f, 0.0f};
    std::array<float, 3> originRotationDegrees{0.0f, 0.0f, 0.0f};
    std::uint32_t selectedOriginRuntimeIndex = kNoSelectedRuntimeIndex;
    std::string originStatusMessage;
    HWND originEditWindow = nullptr;
    WNDPROC originEditOriginalProc = nullptr;
};

} // namespace ovtr::win32
