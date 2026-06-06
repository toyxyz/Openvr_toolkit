#include "platform/win32/ExportProgressWorker.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ExportProgressDialog.h"
#include "platform/win32/WindowLayout.h"

#include <algorithm>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>

namespace ovtr::win32 {
namespace {

void postProgressMessage(HWND hwnd, const UINT message) noexcept
{
    if (hwnd) {
        PostMessageW(hwnd, message, 0, 0);
    }
}

ExportProgressResult failureResult(const std::string& message)
{
    ExportProgressResult result;
    result.statusMessage = message;
    result.logMessages.push_back(message);
    return result;
}

void storeCompletion(AppWindowState& state, const ExportProgressResult& result)
{
    std::lock_guard<std::mutex> lock(state.exportProgressMutex);
    state.exportProgressVisible = false;
    state.exportCompletionPending = true;
    state.exportProgress = 1.0f;
    state.exportProgressDetail.clear();
    state.exportCompletionStatus = result.statusMessage;
    state.exportCompletionLogs = result.logMessages;
}

} // namespace

ExportProgressReporter::ExportProgressReporter(HWND hwnd, AppWindowState& state) noexcept
    : hwnd_(hwnd)
    , state_(&state)
{
}

void ExportProgressReporter::update(const float progress, std::string detail) const
{
    if (!state_) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(state_->exportProgressMutex);
        state_->exportProgress = std::clamp(progress, 0.0f, 1.0f);
        state_->exportProgressDetail = std::move(detail);
    }
    postProgressMessage(hwnd_, kExportProgressUpdatedMessage);
}

bool isExportProgressActive(const AppWindowState& state)
{
    std::lock_guard<std::mutex> lock(state.exportProgressMutex);
    return state.exportProgressVisible || state.exportCompletionPending;
}

bool beginExportProgress(
    HWND hwnd,
    AppWindowState& state,
    std::string title,
    ExportProgressWork work
) {
    if (isExportProgressActive(state)) {
        state.exportStatusMessage = "export already in progress";
        appendDebugLog(state, "Export blocked: export already in progress");
        invalidateStatusPanel(hwnd);
        return false;
    }
    if (state.exportThread.joinable()) {
        state.exportThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(state.exportProgressMutex);
        state.exportProgressVisible = true;
        state.exportCompletionPending = false;
        state.exportProgress = 0.0f;
        state.exportProgressTitle = std::move(title);
        state.exportProgressDetail = "Preparing export";
        state.exportCompletionStatus.clear();
        state.exportCompletionLogs.clear();
    }

    try {
        state.exportThread = std::thread([hwnd, &state, work = std::move(work)]() mutable {
            ExportProgressResult result;
            try {
                result = work(ExportProgressReporter(hwnd, state));
            } catch (const std::exception& error) {
                result = failureResult("export failed: " + std::string(error.what()));
            } catch (...) {
                result = failureResult("export failed: unknown error");
            }
            storeCompletion(state, result);
            postProgressMessage(hwnd, kExportProgressCompletedMessage);
        });
    } catch (const std::exception& error) {
        storeCompletion(state, failureResult("export worker start failed: " + std::string(error.what())));
        postProgressMessage(hwnd, kExportProgressCompletedMessage);
        return false;
    }
    showExportProgressDialog(hwnd, state);
    return true;
}

void completeExportProgress(HWND hwnd, AppWindowState& state)
{
    ExportProgressResult result;
    {
        std::lock_guard<std::mutex> lock(state.exportProgressMutex);
        if (!state.exportCompletionPending) {
            return;
        }
        result.statusMessage = state.exportCompletionStatus;
        result.logMessages = state.exportCompletionLogs;
        state.exportCompletionPending = false;
        state.exportProgressTitle.clear();
        state.exportCompletionStatus.clear();
        state.exportCompletionLogs.clear();
    }
    if (state.exportThread.joinable()) {
        state.exportThread.join();
    }
    hideExportProgressDialog(state);
    if (!result.statusMessage.empty()) {
        state.exportStatusMessage = result.statusMessage;
    }
    for (const std::string& message : result.logMessages) {
        appendDebugLog(state, message);
    }
    invalidateWindowLayout(hwnd);
}

void stopExportProgressWorker(AppWindowState& state) noexcept
{
    if (state.exportThread.joinable()) {
        state.exportThread.join();
    }
    hideExportProgressDialog(state);
}

ExportProgressSnapshot exportProgressSnapshot(const AppWindowState& state)
{
    std::lock_guard<std::mutex> lock(state.exportProgressMutex);
    ExportProgressSnapshot snapshot;
    snapshot.visible = state.exportProgressVisible;
    snapshot.progress = state.exportProgress;
    snapshot.title = state.exportProgressTitle;
    snapshot.detail = state.exportProgressDetail;
    return snapshot;
}

} // namespace ovtr::win32
