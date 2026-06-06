#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ovtr::win32 {

struct AppExportProgressState {
    std::thread exportThread;
    HWND exportProgressWindow = nullptr;
    mutable std::mutex exportProgressMutex;
    bool exportProgressVisible = false;
    bool exportCompletionPending = false;
    float exportProgress = 0.0f;
    std::string exportProgressTitle;
    std::string exportProgressDetail;
    std::string exportCompletionStatus;
    std::vector<std::string> exportCompletionLogs;
};

} // namespace ovtr::win32
