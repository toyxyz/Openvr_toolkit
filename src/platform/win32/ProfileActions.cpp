#include "platform/win32/ProfileActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/MappingActions.h"
#include "platform/win32/ProfileEditor.h"
#include "platform/win32/ProfilePanelLayout.h"
#include "platform/win32/ProfileStore.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"

#include <filesystem>
#include <string>
#include <utility>

namespace ovtr::win32 {
namespace {

void showProfileError(HWND hwnd, const std::string& error)
{
    MessageBoxW(hwnd, widen(error).c_str(), L"Profile", MB_OK | MB_ICONERROR);
}

bool hasExistingFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error);
}

ProfilePanelControlsLayout controlsForClient(
    const AppWindowState& state,
    const int clientWidth,
    const int clientHeight
)
{
    return profileControlsLayoutForPanel(profilePanelLayoutForClient(&state, clientWidth, clientHeight));
}

void refreshProfileUi(HWND hwnd, const AppWindowState& state)
{
    InvalidateRect(hwnd, nullptr, FALSE);
    if (state.glWindow) {
        InvalidateRect(state.glWindow, nullptr, FALSE);
        UpdateWindow(state.glWindow);
    }
}

} // namespace

void saveCurrentProfile(HWND hwnd, AppWindowState& state)
{
    closeProfileEditor(hwnd, state);
    const std::wstring stem = sanitizedProfileFileStem(state.profile.name);
    if (stem.empty()) {
        MessageBoxW(hwnd, L"Profile name cannot be empty.", L"Profile", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string error;
    if (!ensureProfileDirectory(error)) {
        showProfileError(hwnd, error);
        return;
    }

    state.profile.name = stem;
    syncSelectedMappingActorFromControls(state);
    const std::filesystem::path path = profilePathForName(state.profile.name);
    if (hasExistingFile(path)) {
        const int choice = MessageBoxW(
            hwnd,
            L"A profile with this name already exists. Overwrite it?",
            L"Profile",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
        );
        if (choice != IDYES) {
            appendDebugLog(state, L"Profile save canceled");
            return;
        }
    }

    if (!saveProfileToPath(state.profile, path, error)) {
        showProfileError(hwnd, error);
        return;
    }
    appendDebugLog(state, L"Profile saved: " + path.filename().wstring());
    refreshProfileUi(hwnd, state);
}

void loadProfileFromDialog(HWND hwnd, AppWindowState& state)
{
    closeProfileEditor(hwnd, state);
    std::string error;
    if (!ensureProfileDirectory(error)) {
        showProfileError(hwnd, error);
        return;
    }

    std::filesystem::path selectedPath;
    if (!chooseProfileFile(hwnd, profileDirectoryPath(), selectedPath)) {
        appendDebugLog(state, L"Profile load canceled");
        return;
    }

    BodyProfile loaded;
    if (!loadProfileFromPath(selectedPath, loaded, error)) {
        showProfileError(hwnd, error);
        return;
    }

    state.profile = std::move(loaded);
    syncSelectedMappingActorFromControls(state);
    appendDebugLog(state, L"Profile loaded: " + selectedPath.filename().wstring());
    refreshProfileUi(hwnd, state);
}

bool handleProfilePanelClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    if (!state.profilePanelVisible) {
        return false;
    }

    const ProfilePanelControlsLayout controls = controlsForClient(state, clientWidth, clientHeight);
    if (!controls.valid) {
        return false;
    }
    if (PtInRect(&controls.previewButtonRect, point)) {
        state.profilePreviewEnabled = !state.profilePreviewEnabled;
        appendDebugLog(state, state.profilePreviewEnabled ? L"Profile preview enabled" : L"Profile preview disabled");
        refreshProfileUi(hwnd, state);
        return true;
    }
    if (PtInRect(&controls.saveButtonRect, point)) {
        saveCurrentProfile(hwnd, state);
        return true;
    }
    if (PtInRect(&controls.loadButtonRect, point)) {
        loadProfileFromDialog(hwnd, state);
        return true;
    }
    return false;
}

bool handleProfilePanelDoubleClick(
    HWND hwnd,
    AppWindowState& state,
    const int clientWidth,
    const int clientHeight,
    const POINT point
)
{
    if (!state.profilePanelVisible) {
        return false;
    }

    const ProfilePanelFieldLayout field = profileFieldLayoutAtPoint(
        profilePanelLayoutForClient(&state, clientWidth, clientHeight),
        point,
        state.profileScrollOffset
    );
    if (!field.valid) {
        return false;
    }

    showProfileEditor(hwnd, state, field.target);
    return true;
}

} // namespace ovtr::win32
