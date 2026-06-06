#include "platform/win32/SessionSaveActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/RecordingSessionList.h"
#include "platform/win32/SessionMappingSnapshot.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "recording/SessionManifest.h"

#include <filesystem>
#include <string>
#include <system_error>

namespace ovtr::win32 {
namespace {

bool samePath(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
{
    std::error_code error;
    const auto left = std::filesystem::weakly_canonical(lhs, error);
    if (error) {
        return lhs.lexically_normal() == rhs.lexically_normal();
    }
    error.clear();
    const auto right = std::filesystem::weakly_canonical(rhs, error);
    return error ? lhs.lexically_normal() == rhs.lexically_normal() : left == right;
}

bool ensureRecordingsRoot(const std::filesystem::path& root, std::string& error)
{
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) {
        error = "failed to create recordings folder: " + ec.message();
        return false;
    }
    return true;
}

bool copySessionFolder(const std::filesystem::path& source, const std::filesystem::path& target, std::string& error)
{
    std::error_code ec;
    std::filesystem::copy(source, target, std::filesystem::copy_options::recursive, ec);
    if (ec) {
        error = "failed to copy session folder: " + ec.message();
        return false;
    }
    return true;
}

bool removeExistingTarget(const std::filesystem::path& target, std::string& error)
{
    std::error_code ec;
    std::filesystem::remove_all(target, ec);
    if (ec) {
        error = "failed to overwrite existing session folder: " + ec.message();
        return false;
    }
    return true;
}

bool writeSavedManifest(AppWindowState& state, const std::filesystem::path& target, std::string& error)
{
    RecordingSession session = state.loadedSession;
    session.sessionId = target.filename().string();
    session.sessionName = session.sessionId;
    session.framesPath = target / "frames.bin";
    session.frameIndexPath = target / "frame_index.bin";
    if (!writeManifestJson(session, state.loadedSessionStats, target / "manifest.json", error)) {
        return false;
    }
    state.loadedSession = std::move(session);
    return true;
}

bool confirmOverwrite(HWND hwnd, const std::filesystem::path& target)
{
    const std::wstring message =
        L"A session with this name already exists.\n\n" +
        target.filename().wstring() +
        L"\n\nOverwrite it?";
    return MessageBoxW(hwnd, message.c_str(), L"Save Session", MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2) == IDOK;
}

std::wstring currentSessionNameInput(const AppWindowState& state)
{
    if (state.sessionEditWindow && IsWindow(state.sessionEditWindow)) {
        return trimWide(readWindowText(state.sessionEditWindow));
    }
    return state.sessionName;
}

std::wstring targetSessionFolderName(const AppWindowState& state, const std::filesystem::path& source)
{
    const std::wstring fromInput = sanitizedSessionFolderName(currentSessionNameInput(state));
    return fromInput.empty() ? source.filename().wstring() : fromInput;
}

} // namespace

bool saveLoadedSessionFolder(HWND hwnd, AppWindowState& state)
{
    if (!state.loadedSessionActive || state.loadedSessionFolder.empty()) {
        appendDebugLog(state, L"Save session ignored: no session is loaded");
        MessageBoxW(hwnd, L"Load a session before saving it.", L"Save Session", MB_OK | MB_ICONWARNING);
        return false;
    }

    const std::filesystem::path root = recordingSessionsRootPath();
    std::string error;
    if (!ensureRecordingsRoot(root, error)) {
        appendDebugLog(state, "Save session failed: " + error);
        MessageBoxW(hwnd, widen(error).c_str(), L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }

    const std::filesystem::path source = state.loadedSessionFolder;
    if (source.filename().empty()) {
        appendDebugLog(state, L"Save session failed: loaded session folder has no name");
        MessageBoxW(hwnd, L"Loaded session folder has no name.", L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }
    const std::filesystem::path target = root / targetSessionFolderName(state, source);
    std::error_code existsError;
    const bool targetExists = std::filesystem::exists(target, existsError);
    if (existsError) {
        error = "failed to inspect target session folder: " + existsError.message();
        appendDebugLog(state, "Save session failed: " + error);
        MessageBoxW(hwnd, widen(error).c_str(), L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }

    const bool sameFolder = targetExists && samePath(source, target);
    if (targetExists && !confirmOverwrite(hwnd, target)) {
        appendDebugLog(state, L"Save session canceled");
        return false;
    }
    if (targetExists && !sameFolder && !removeExistingTarget(target, error)) {
        appendDebugLog(state, "Save session failed: " + error);
        MessageBoxW(hwnd, widen(error).c_str(), L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!sameFolder && !copySessionFolder(source, target, error)) {
        appendDebugLog(state, "Save session failed: " + error);
        MessageBoxW(hwnd, widen(error).c_str(), L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!writeSavedManifest(state, target, error)) {
        appendDebugLog(state, "Save session failed: " + error);
        MessageBoxW(hwnd, widen(error).c_str(), L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!saveSessionMappingSnapshot(state, state.devices, target, error)) {
        appendDebugLog(state, "Save session failed: " + error);
        MessageBoxW(hwnd, widen(error).c_str(), L"Save Session", MB_OK | MB_ICONERROR);
        return false;
    }

    state.loadedSessionFolder = target;
    state.selectedSessionName = target.filename().wstring();
    state.sessionName = state.selectedSessionName;
    appendDebugLog(state, L"Session saved: " + state.selectedSessionName);
    invalidateWindowLayout(hwnd);
    return true;
}

} // namespace ovtr::win32
