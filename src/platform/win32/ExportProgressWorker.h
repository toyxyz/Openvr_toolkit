#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <functional>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct AppWindowState;

inline constexpr UINT kExportProgressUpdatedMessage = WM_APP + 2;
inline constexpr UINT kExportProgressCompletedMessage = WM_APP + 3;

struct ExportProgressResult {
    std::string statusMessage;
    std::vector<std::string> logMessages;
};

struct ExportProgressSnapshot {
    bool visible = false;
    float progress = 0.0f;
    std::string title;
    std::string detail;
};

class ExportProgressReporter {
public:
    ExportProgressReporter(HWND hwnd, AppWindowState& state) noexcept;
    void update(float progress, std::string detail) const;

private:
    HWND hwnd_ = nullptr;
    AppWindowState* state_ = nullptr;
};

using ExportProgressWork = std::function<ExportProgressResult(const ExportProgressReporter&)>;

bool isExportProgressActive(const AppWindowState& state);
bool beginExportProgress(
    HWND hwnd,
    AppWindowState& state,
    std::string title,
    ExportProgressWork work
);
void completeExportProgress(HWND hwnd, AppWindowState& state);
void stopExportProgressWorker(AppWindowState& state) noexcept;
ExportProgressSnapshot exportProgressSnapshot(const AppWindowState& state);

} // namespace ovtr::win32
