#include "platform/win32/SessionEditor.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kSessionEditControlId = 2101;

RECT sessionEditorRectForClient(AppWindowState& state, const int clientWidth, const int clientHeight)
{
    const ViewportControlLayout layout = viewportControlLayoutForClient(&state, clientWidth, clientHeight);
    if (!layout.valid) {
        return RECT{0, 0, 0, 0};
    }
    RECT rect = layout.sessionValueRect;
    rect.left += 4;
    rect.right -= 4;
    return rect;
}

bool applySessionEditorText(HWND hwnd, AppWindowState& state)
{
    if (!state.sessionEditWindow || !IsWindow(state.sessionEditWindow)) {
        return false;
    }

    state.sessionName = trimWide(readWindowText(state.sessionEditWindow));
    appendDebugLog(
        state,
        state.sessionName.empty() ? L"Session cleared" : L"Session set: " + state.sessionName
    );
    closeSessionEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

LRESULT CALLBACK sessionEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;

    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applySessionEditorText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeSessionEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applySessionEditorText(parent, *state);
        return 0;
    }

    if (state && state->sessionEditOriginalProc) {
        return CallWindowProcW(state->sessionEditOriginalProc, hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

void closeSessionEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.sessionEditWindow;
    if (!editWindow) {
        return;
    }

    if (state.sessionEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.sessionEditOriginalProc));
    }
    state.sessionEditWindow = nullptr;
    state.sessionEditOriginalProc = nullptr;
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showSessionEditor(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = sessionEditorRectForClient(
        state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }

    if (state.sessionEditWindow && IsWindow(state.sessionEditWindow)) {
        SetWindowTextW(state.sessionEditWindow, state.sessionName.c_str());
        MoveWindow(
            state.sessionEditWindow,
            editRect.left,
            editRect.top,
            editRect.right - editRect.left,
            editRect.bottom - editRect.top,
            TRUE
        );
        ShowWindow(state.sessionEditWindow, SW_SHOW);
        focusAndSelectAllText(state.sessionEditWindow);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.sessionEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        state.sessionName.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kSessionEditControlId),
        instance,
        nullptr
    );
    if (!state.sessionEditWindow) {
        return;
    }

    SendMessageW(
        state.sessionEditWindow,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
        TRUE
    );
    state.sessionEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(state.sessionEditWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(sessionEditProc))
    );
    focusAndSelectAllText(state.sessionEditWindow);
    appendDebugLog(state, L"Session editor opened");
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateSessionEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.sessionEditWindow || !IsWindow(state.sessionEditWindow)) {
        state.sessionEditWindow = nullptr;
        state.sessionEditOriginalProc = nullptr;
        return;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = sessionEditorRectForClient(
        state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.sessionEditWindow, SW_HIDE);
        return;
    }

    MoveWindow(
        state.sessionEditWindow,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        TRUE
    );
    ShowWindow(state.sessionEditWindow, SW_SHOW);
}

} // namespace ovtr::win32
