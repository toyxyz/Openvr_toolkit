#include "platform/win32/MappingOffsetPresetNameEditor.h"

#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/MappingEditPanelLayout.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kOffsetPresetNameEditControlId = 2302;

RECT presetNameEditorRect(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    return mappingEditPanelLayoutForPanel(panelLayout).presetNameEditRect;
}

void applyEditorText(HWND hwnd, AppWindowState& state)
{
    syncMappingOffsetPresetNameEditorText(state);
    closeMappingOffsetPresetNameEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void cancelEditor(HWND hwnd, AppWindowState& state)
{
    state.mappingEditOffsetPresetName = state.mappingEditOffsetPresetNameSnapshot;
    closeMappingOffsetPresetNameEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
}

LRESULT CALLBACK offsetPresetNameEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;
    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyEditorText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            cancelEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applyEditorText(parent, *state);
        return 0;
    }
    if (state && state->mappingEditOffsetPresetNameEditOriginalProc) {
        return CallWindowProcW(
            state->mappingEditOffsetPresetNameEditOriginalProc,
            hwnd,
            message,
            wparam,
            lparam
        );
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

void syncMappingOffsetPresetNameEditorText(AppWindowState& state)
{
    if (state.mappingEditOffsetPresetNameEditWindow && IsWindow(state.mappingEditOffsetPresetNameEditWindow)) {
        state.mappingEditOffsetPresetName = trimWide(readWindowText(state.mappingEditOffsetPresetNameEditWindow));
    }
}

void closeMappingOffsetPresetNameEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.mappingEditOffsetPresetNameEditWindow;
    if (!editWindow) {
        return;
    }
    if (state.mappingEditOffsetPresetNameEditOriginalProc) {
        SetWindowLongPtrW(
            editWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(state.mappingEditOffsetPresetNameEditOriginalProc)
        );
    }
    state.mappingEditOffsetPresetNameEditWindow = nullptr;
    state.mappingEditOffsetPresetNameEditOriginalProc = nullptr;
    state.mappingEditOffsetPresetNameSnapshot.clear();
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showMappingOffsetPresetNameEditor(HWND hwnd, AppWindowState& state)
{
    const RECT editRect = presetNameEditorRect(hwnd, state);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }
    if (state.mappingEditOffsetPresetNameEditWindow) {
        focusAndSelectAllText(state.mappingEditOffsetPresetNameEditWindow);
        return;
    }
    state.mappingEditOffsetPresetNameSnapshot = state.mappingEditOffsetPresetName;
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.mappingEditOffsetPresetNameEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        state.mappingEditOffsetPresetName.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kOffsetPresetNameEditControlId),
        instance,
        nullptr
    );
    if (!state.mappingEditOffsetPresetNameEditWindow) {
        state.mappingEditOffsetPresetNameSnapshot.clear();
        return;
    }
    SendMessageW(
        state.mappingEditOffsetPresetNameEditWindow,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
        TRUE
    );
    state.mappingEditOffsetPresetNameEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(
            state.mappingEditOffsetPresetNameEditWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(offsetPresetNameEditProc)
        )
    );
    focusAndSelectAllText(state.mappingEditOffsetPresetNameEditWindow);
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateMappingOffsetPresetNameEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.mappingEditOffsetPresetNameEditWindow || !IsWindow(state.mappingEditOffsetPresetNameEditWindow)) {
        state.mappingEditOffsetPresetNameEditWindow = nullptr;
        state.mappingEditOffsetPresetNameEditOriginalProc = nullptr;
        return;
    }
    const RECT editRect = presetNameEditorRect(hwnd, state);
    if (!state.editPanelVisible || editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.mappingEditOffsetPresetNameEditWindow, SW_HIDE);
        return;
    }
    MoveWindow(
        state.mappingEditOffsetPresetNameEditWindow,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        TRUE
    );
    ShowWindow(state.mappingEditOffsetPresetNameEditWindow, SW_SHOW);
}

} // namespace ovtr::win32
