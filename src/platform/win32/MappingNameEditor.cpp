#include "platform/win32/MappingNameEditor.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/MappingPanelLayout.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kMappingNameEditControlId = 2202;

std::wstring effectiveMappingName(const AppWindowState& state)
{
    return state.mappingPresetName.empty() ? state.profile.name : state.mappingPresetName;
}

RECT mappingNameEditorRect(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const ProfilePanelLayout panelLayout = profilePanelLayoutForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    return mappingControlsLayoutForPanel(panelLayout).nameValueRect;
}

bool applyMappingNameEditorText(HWND hwnd, AppWindowState& state)
{
    if (!state.mappingNameEditWindow || !IsWindow(state.mappingNameEditWindow)) {
        return false;
    }
    const std::wstring text = trimWide(readWindowText(state.mappingNameEditWindow));
    if (text.empty()) {
        MessageBoxW(hwnd, L"Mapping name cannot be empty.", L"Mapping", MB_OK | MB_ICONWARNING);
        focusAndSelectAllText(state.mappingNameEditWindow);
        return false;
    }
    state.mappingPresetName = text;
    appendDebugLog(state, L"Mapping name updated");
    closeMappingNameEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

void cancelMappingNameEditor(HWND hwnd, AppWindowState& state)
{
    state.mappingPresetName = state.mappingNameEditSnapshot;
    appendDebugLog(state, L"Mapping name edit canceled");
    closeMappingNameEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
}

LRESULT CALLBACK mappingNameEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;
    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyMappingNameEditorText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            cancelMappingNameEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applyMappingNameEditorText(parent, *state);
        return 0;
    }
    if (state && state->mappingNameEditOriginalProc) {
        return CallWindowProcW(state->mappingNameEditOriginalProc, hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

void closeMappingNameEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.mappingNameEditWindow;
    if (!editWindow) {
        return;
    }
    if (state.mappingNameEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.mappingNameEditOriginalProc));
    }
    state.mappingNameEditWindow = nullptr;
    state.mappingNameEditOriginalProc = nullptr;
    state.mappingNameEditSnapshot.clear();
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showMappingNameEditor(HWND hwnd, AppWindowState& state)
{
    const RECT editRect = mappingNameEditorRect(hwnd, state);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }
    state.mappingNameEditSnapshot = state.mappingPresetName;
    const std::wstring text = effectiveMappingName(state);

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.mappingNameEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kMappingNameEditControlId),
        instance,
        nullptr
    );
    if (!state.mappingNameEditWindow) {
        state.mappingNameEditSnapshot.clear();
        return;
    }
    SendMessageW(state.mappingNameEditWindow, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    state.mappingNameEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(state.mappingNameEditWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(mappingNameEditProc))
    );
    focusAndSelectAllText(state.mappingNameEditWindow);
    appendDebugLog(state, L"Mapping name editor opened");
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateMappingNameEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.mappingNameEditWindow || !IsWindow(state.mappingNameEditWindow)) {
        state.mappingNameEditWindow = nullptr;
        state.mappingNameEditOriginalProc = nullptr;
        return;
    }
    const RECT editRect = mappingNameEditorRect(hwnd, state);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.mappingNameEditWindow, SW_HIDE);
        return;
    }
    MoveWindow(state.mappingNameEditWindow, editRect.left, editRect.top,
               editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
    ShowWindow(state.mappingNameEditWindow, SW_SHOW);
}

} // namespace ovtr::win32
