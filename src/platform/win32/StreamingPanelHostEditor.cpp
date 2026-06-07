#include "platform/win32/StreamingPanelEditor.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <string>

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kStreamingHostEditControlId = 2400;

std::wstring hostText(const std::string& host)
{
    return widen(host);
}

RECT streamingHostEditorRectForClient(AppWindowState& state, const int width, const int height)
{
    const StreamingPanelLayout layout = streamingPanelLayoutForClient(&state, width, height);
    if (!layout.valid || !layout.vmcVisible) {
        return RECT{0, 0, 0, 0};
    }
    RECT rect = layout.hostValueRect;
    rect.left += 4;
    rect.right -= 4;
    return rect;
}

bool applyStreamingHostText(HWND hwnd, AppWindowState& state)
{
    if (!state.vmcSendHostEditWindow || !IsWindow(state.vmcSendHostEditWindow)) {
        return false;
    }
    const std::wstring text = trimWide(readWindowText(state.vmcSendHostEditWindow));
    if (text.empty()) {
        appendDebugLog(state, L"Invalid VMC streaming host: empty");
        SetWindowTextW(state.vmcSendHostEditWindow, hostText(state.vmcSendHost).c_str());
        focusAndSelectAllText(state.vmcSendHostEditWindow);
        return false;
    }
    const std::string host = narrow(text);
    state.vmcSendHost = host;
    if (state.vmcSender.running() && state.vmcSender.host() != host) {
        state.vmcSender.stop();
    }
    appendDebugLog(state, L"VMC streaming host set: " + text);
    closeStreamingHostEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

LRESULT CALLBACK streamingHostEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;
    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyStreamingHostText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeStreamingHostEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applyStreamingHostText(parent, *state);
        return 0;
    }
    if (state && state->vmcSendHostEditOriginalProc) {
        return CallWindowProcW(state->vmcSendHostEditOriginalProc, hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

void closeStreamingHostEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.vmcSendHostEditWindow;
    if (!editWindow) {
        return;
    }
    if (state.vmcSendHostEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.vmcSendHostEditOriginalProc));
    }
    state.vmcSendHostEditWindow = nullptr;
    state.vmcSendHostEditOriginalProc = nullptr;
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showStreamingHostEditor(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = streamingHostEditorRectForClient(state, clientRect.right, clientRect.bottom);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }

    if (state.vmcSendHostEditWindow && IsWindow(state.vmcSendHostEditWindow)) {
        SetWindowTextW(state.vmcSendHostEditWindow, hostText(state.vmcSendHost).c_str());
        MoveWindow(state.vmcSendHostEditWindow, editRect.left, editRect.top,
            editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
        ShowWindow(state.vmcSendHostEditWindow, SW_SHOW);
        focusAndSelectAllText(state.vmcSendHostEditWindow);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.vmcSendHostEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        hostText(state.vmcSendHost).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kStreamingHostEditControlId),
        instance,
        nullptr
    );
    if (!state.vmcSendHostEditWindow) {
        appendDebugLog(state, L"Failed to open VMC streaming host editor");
        return;
    }

    applyControlFont(state.vmcSendHostEditWindow, reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    state.vmcSendHostEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(state.vmcSendHostEditWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(streamingHostEditProc))
    );
    focusAndSelectAllText(state.vmcSendHostEditWindow);
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateStreamingHostEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.vmcSendHostEditWindow || !IsWindow(state.vmcSendHostEditWindow)) {
        state.vmcSendHostEditWindow = nullptr;
        state.vmcSendHostEditOriginalProc = nullptr;
        return;
    }
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = streamingHostEditorRectForClient(state, clientRect.right, clientRect.bottom);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.vmcSendHostEditWindow, SW_HIDE);
        return;
    }
    MoveWindow(state.vmcSendHostEditWindow, editRect.left, editRect.top,
        editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
    ShowWindow(state.vmcSendHostEditWindow, SW_SHOW);
}

} // namespace ovtr::win32
