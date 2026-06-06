#include "platform/win32/SessionActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/RecordingCleanupActions.h"
#include "platform/win32/RecordingSessionList.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "platform/win32/RuntimeStatus.h"
#include "platform/win32/SessionPlayback.h"
#include "platform/win32/ViewportRenderer.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "recording/RecordingState.h"

#include <filesystem>
#include <system_error>
#include <vector>

namespace ovtr::win32 {
namespace {

bool samePath(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
{
    std::error_code error;
    const std::filesystem::path left = std::filesystem::weakly_canonical(lhs, error);
    if (error) {
        return lhs.lexically_normal() == rhs.lexically_normal();
    }
    error.clear();
    const std::filesystem::path right = std::filesystem::weakly_canonical(rhs, error);
    if (error) {
        return lhs.lexically_normal() == rhs.lexically_normal();
    }
    return left == right;
}

const RecordingSessionListRow* selectedSessionRow(
    const std::vector<RecordingSessionListRow>& rows,
    const std::wstring& selectedName
)
{
    for (const RecordingSessionListRow& row : rows) {
        if (row.name == selectedName) {
            return &row;
        }
    }
    return nullptr;
}

bool activeRecorderState(const ovtr::RecorderState state) noexcept
{
    return state == ovtr::RecorderState::Starting ||
        state == ovtr::RecorderState::Recording ||
        state == ovtr::RecorderState::Paused ||
        state == ovtr::RecorderState::Stopping ||
        state == ovtr::RecorderState::Finalizing;
}

void showDeleteSessionBlockedMessage(HWND hwnd, const std::wstring& message)
{
    MessageBoxW(hwnd, message.c_str(), L"Delete Session", MB_OK | MB_ICONWARNING);
}

} // namespace

bool loadSelectedSessionFolder(HWND hwnd, AppWindowState& state)
{
    const std::filesystem::path root = activeSessionDirectoryPath(state);
    const std::vector<RecordingSessionListRow> rows =
        listRecordingSessionFolders(root);
    const RecordingSessionListRow* row = selectedSessionRow(rows, state.selectedSessionName);
    if (!row) {
        appendDebugLog(state, L"Load session ignored: no session selected");
        return false;
    }
    if (activeRecorderState(state.recorder.state())) {
        appendDebugLog(state, L"Load session blocked: recorder is active");
        return false;
    }
    if (state.recordingDelayActive) {
        appendDebugLog(state, L"Load session blocked: recording delay is active");
        return false;
    }

    stopPoseSamplingWorker(state);

    std::string error;
    if (!openLoadedSession(state, row->folder, error)) {
        appendDebugLog(state, "Load session failed: " + error);
        startPoseSamplingWorker(hwnd, state);
        refreshStatus(hwnd, true);
        return false;
    }
    state.sessionName = row->name;
    if (state.sessionEditWindow && IsWindow(state.sessionEditWindow)) {
        SetWindowTextW(state.sessionEditWindow, state.sessionName.c_str());
    }

    appendDebugLog(state, L"Session loaded: " + row->name);
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    return true;
}

bool closeLoadedSessionFolder(HWND hwnd, AppWindowState& state)
{
    if (!closeLoadedSession(state)) {
        appendDebugLog(state, L"Close loaded session ignored: no session is loaded");
        return false;
    }
    state.devices.clear();
    state.poses = {};
    if (state.sessionEditWindow && IsWindow(state.sessionEditWindow)) {
        SetWindowTextW(state.sessionEditWindow, state.sessionName.c_str());
    }
    startPoseSamplingWorker(hwnd, state);
    refreshStatus(hwnd, true);
    appendDebugLog(state, L"Loaded session closed; live tracking resumed");
    layoutChildWindows(hwnd);
    invalidateWindowLayout(hwnd);
    if (state.glWindow) {
        renderViewport(state.glWindow);
    }
    return true;
}

bool deleteSelectedSessionFolder(HWND hwnd, AppWindowState& state)
{
    const std::filesystem::path root = activeSessionDirectoryPath(state);
    const std::vector<RecordingSessionListRow> rows =
        listRecordingSessionFolders(root);
    const RecordingSessionListRow* row = selectedSessionRow(rows, state.selectedSessionName);
    if (!row) {
        appendDebugLog(state, L"Delete session ignored: no session selected");
        showDeleteSessionBlockedMessage(hwnd, L"Select a session before deleting it.");
        return false;
    }

    if (!state.currentSessionFolder.empty() &&
        samePath(row->folder, state.currentSessionFolder) &&
        activeRecorderState(state.recorder.state())) {
        appendDebugLog(state, L"Delete session blocked: recording is using " + row->name);
        showDeleteSessionBlockedMessage(
            hwnd,
            L"This session is being used by the active recording.\n\nStop recording before deleting it."
        );
        return false;
    }
    if (state.loadedSessionActive && samePath(row->folder, state.loadedSessionFolder)) {
        appendDebugLog(state, L"Delete session blocked: session is loaded " + row->name);
        showDeleteSessionBlockedMessage(
            hwnd,
            L"This session is currently loaded.\n\nClose the loaded session before deleting it."
        );
        return false;
    }

    const std::wstring prompt = L"Delete session folder?\n\n" + row->name + L"\n\nThis cannot be undone.";
    const int answer = MessageBoxW(
        hwnd,
        prompt.c_str(),
        L"Delete Session",
        MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2
    );
    if (answer != IDOK) {
        appendDebugLog(state, L"Delete session canceled: " + row->name);
        return false;
    }

    std::filesystem::path folder = row->folder;
    std::string message;
    const bool deleted = deleteTemporarySessionFolder(folder, root, message);
    if (!deleted) {
        appendDebugLog(state, "Delete session failed: " + message);
        MessageBoxW(hwnd, widen(message).c_str(), L"Delete Session", MB_OK | MB_ICONERROR);
        return false;
    }

    if (samePath(row->folder, state.currentSessionFolder)) {
        state.currentSessionFolder.clear();
    }
    state.selectedSessionName.clear();
    appendDebugLog(state, message.empty() ? "Session folder deleted" : message);
    invalidateWindowLayout(hwnd);
    return true;
}

} // namespace ovtr::win32
