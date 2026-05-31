#include "platform/win32/ProfileEditor.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/ProfileEditModel.h"
#include "platform/win32/ProfilePanelLayout.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <vector>

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kProfileEditControlId = 2201;

void refreshProfileWindows(HWND hwnd, const AppWindowState& state)
{
    InvalidateRect(hwnd, nullptr, FALSE);
    if (state.glWindow) {
        InvalidateRect(state.glWindow, nullptr, FALSE);
        UpdateWindow(state.glWindow);
    }
}

RECT editorRectForTarget(AppWindowState& state, const int clientWidth, const int clientHeight)
{
    const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(&state, clientWidth, clientHeight);
    const std::vector<ProfilePanelFieldLayout> fields =
        profileFieldLayoutsForPanel(panelLayout, state.profileScrollOffset);
    for (const ProfilePanelFieldLayout& field : fields) {
        if (field.target.kind == state.profileEditTarget.kind &&
            field.target.measurementIndex == state.profileEditTarget.measurementIndex) {
            return field.valueRect;
        }
    }
    return RECT{0, 0, 0, 0};
}

bool applyProfileEditorText(HWND hwnd, AppWindowState& state)
{
    if (!state.profileEditWindow || !IsWindow(state.profileEditWindow)) {
        return false;
    }

    const std::wstring text = readWindowText(state.profileEditWindow);
    std::wstring errorMessage;
    if (!applyProfileEditCommittedText(state.profile, state.profileEditTarget, text, errorMessage)) {
        MessageBoxW(hwnd, errorMessage.c_str(), L"Profile", MB_OK | MB_ICONWARNING);
        focusAndSelectAllText(state.profileEditWindow);
        return false;
    }

    appendDebugLog(state, L"Profile value updated");
    closeProfileEditor(hwnd, state);
    refreshProfileWindows(hwnd, state);
    return true;
}

void applyProfileEditorLiveText(HWND hwnd, AppWindowState& state)
{
    if (!state.profileEditWindow || !IsWindow(state.profileEditWindow)) {
        return;
    }
    if (applyProfileEditLiveText(state.profile, state.profileEditTarget, readWindowText(state.profileEditWindow))) {
        refreshProfileWindows(hwnd, state);
    }
}

void cancelProfileEditor(HWND hwnd, AppWindowState& state)
{
    restoreProfileEditSnapshot(state);
    appendDebugLog(state, L"Profile edit canceled");
    closeProfileEditor(hwnd, state);
    refreshProfileWindows(hwnd, state);
}

bool shouldApplyLiveText(const UINT message, const WPARAM wparam) noexcept
{
    return message == WM_CHAR ||
        message == WM_PASTE ||
        message == WM_CUT ||
        message == WM_CLEAR ||
        message == WM_UNDO ||
        (message == WM_KEYDOWN && wparam == VK_DELETE);
}

LRESULT CALLBACK profileEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;

    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyProfileEditorText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            cancelProfileEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applyProfileEditorText(parent, *state);
        return 0;
    }

    if (state && state->profileEditOriginalProc) {
        const LRESULT result = CallWindowProcW(state->profileEditOriginalProc, hwnd, message, wparam, lparam);
        if (parent && shouldApplyLiveText(message, wparam)) {
            applyProfileEditorLiveText(parent, *state);
        }
        return result;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

void closeProfileEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.profileEditWindow;
    if (!editWindow) {
        return;
    }

    if (state.profileEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.profileEditOriginalProc));
    }
    state.profileEditWindow = nullptr;
    state.profileEditOriginalProc = nullptr;
    state.profileEditTarget = {};
    clearProfileEditSnapshot(state);
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showProfileEditor(HWND hwnd, AppWindowState& state, const ProfileEditTarget target)
{
    if (!profileEditTargetIsValid(target)) {
        return;
    }
    state.profileEditTarget = target;
    beginProfileEditSnapshot(state);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = editorRectForTarget(
        state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        clearProfileEditSnapshot(state);
        state.profileEditTarget = {};
        return;
    }

    const std::wstring text = profileTextForTarget(state.profile, target);
    if (state.profileEditWindow && IsWindow(state.profileEditWindow)) {
        SetWindowTextW(state.profileEditWindow, text.c_str());
        MoveWindow(
            state.profileEditWindow,
            editRect.left,
            editRect.top,
            editRect.right - editRect.left,
            editRect.bottom - editRect.top,
            TRUE
        );
        ShowWindow(state.profileEditWindow, SW_SHOW);
        focusAndSelectAllText(state.profileEditWindow);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.profileEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kProfileEditControlId),
        instance,
        nullptr
    );
    if (!state.profileEditWindow) {
        clearProfileEditSnapshot(state);
        state.profileEditTarget = {};
        return;
    }

    SendMessageW(
        state.profileEditWindow,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
        TRUE
    );
    state.profileEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(state.profileEditWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(profileEditProc))
    );
    focusAndSelectAllText(state.profileEditWindow);
    appendDebugLog(state, L"Profile editor opened");
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateProfileEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.profileEditWindow || !IsWindow(state.profileEditWindow)) {
        state.profileEditWindow = nullptr;
        state.profileEditOriginalProc = nullptr;
        state.profileEditTarget = {};
        clearProfileEditSnapshot(state);
        return;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = editorRectForTarget(
        state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.profileEditWindow, SW_HIDE);
        return;
    }

    MoveWindow(
        state.profileEditWindow,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        TRUE
    );
    ShowWindow(state.profileEditWindow, SW_SHOW);
}

} // namespace ovtr::win32
