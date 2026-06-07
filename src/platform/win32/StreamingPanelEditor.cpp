#include "platform/win32/StreamingPanelEditor.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/Win32String.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <exception>
#include <string>

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kStreamingPortEditControlId = 2401;

std::wstring portText(const int port)
{
    return std::to_wstring(port);
}

RECT streamingPortEditorRectForClient(AppWindowState& state, const int width, const int height)
{
    const StreamingPanelLayout layout = streamingPanelLayoutForClient(&state, width, height);
    if (!layout.valid || !layout.vmcVisible) {
        return RECT{0, 0, 0, 0};
    }
    RECT rect = layout.portValueRect;
    rect.left += 4;
    rect.right -= 4;
    return rect;
}

bool tryParsePort(const std::wstring& text, int& port)
{
    try {
        std::size_t parsed = 0;
        const int value = std::stoi(text, &parsed);
        if (parsed != text.size() || value < 1 || value > 65535) {
            return false;
        }
        port = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool applyStreamingPortText(HWND hwnd, AppWindowState& state)
{
    if (!state.vmcSendPortEditWindow || !IsWindow(state.vmcSendPortEditWindow)) {
        return false;
    }
    int port = state.vmcSendPort;
    const std::wstring text = trimWide(readWindowText(state.vmcSendPortEditWindow));
    if (!tryParsePort(text, port)) {
        appendDebugLog(state, L"Invalid VMC streaming port: " + text);
        SetWindowTextW(state.vmcSendPortEditWindow, portText(state.vmcSendPort).c_str());
        focusAndSelectAllText(state.vmcSendPortEditWindow);
        return false;
    }
    state.vmcSendPort = port;
    if (state.vmcSender.running() && state.vmcSender.port() != port) {
        state.vmcSender.stop();
    }
    appendDebugLog(state, L"VMC streaming port set: " + portText(port));
    closeStreamingPortEditor(hwnd, state);
    InvalidateRect(hwnd, nullptr, FALSE);
    return true;
}

LRESULT CALLBACK streamingPortEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;
    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyStreamingPortText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeStreamingPortEditor(parent, *state);
            return 0;
        }
    }
    if (message == WM_KILLFOCUS && state) {
        applyStreamingPortText(parent, *state);
        return 0;
    }
    if (state && state->vmcSendPortEditOriginalProc) {
        return CallWindowProcW(state->vmcSendPortEditOriginalProc, hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

void closeStreamingPortEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.vmcSendPortEditWindow;
    if (!editWindow) {
        return;
    }
    if (state.vmcSendPortEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.vmcSendPortEditOriginalProc));
    }
    state.vmcSendPortEditWindow = nullptr;
    state.vmcSendPortEditOriginalProc = nullptr;
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showStreamingPortEditor(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = streamingPortEditorRectForClient(state, clientRect.right, clientRect.bottom);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }

    if (state.vmcSendPortEditWindow && IsWindow(state.vmcSendPortEditWindow)) {
        SetWindowTextW(state.vmcSendPortEditWindow, portText(state.vmcSendPort).c_str());
        MoveWindow(state.vmcSendPortEditWindow, editRect.left, editRect.top,
            editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
        ShowWindow(state.vmcSendPortEditWindow, SW_SHOW);
        focusAndSelectAllText(state.vmcSendPortEditWindow);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.vmcSendPortEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        portText(state.vmcSendPort).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kStreamingPortEditControlId),
        instance,
        nullptr
    );
    if (!state.vmcSendPortEditWindow) {
        appendDebugLog(state, L"Failed to open VMC streaming port editor");
        return;
    }

    applyControlFont(state.vmcSendPortEditWindow, reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    state.vmcSendPortEditOriginalProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(state.vmcSendPortEditWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(streamingPortEditProc))
    );
    focusAndSelectAllText(state.vmcSendPortEditWindow);
    InvalidateRect(hwnd, &editRect, FALSE);
}

void updateStreamingPortEditorLayout(HWND hwnd, AppWindowState& state)
{
    if (!state.vmcSendPortEditWindow || !IsWindow(state.vmcSendPortEditWindow)) {
        state.vmcSendPortEditWindow = nullptr;
        state.vmcSendPortEditOriginalProc = nullptr;
        return;
    }
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = streamingPortEditorRectForClient(state, clientRect.right, clientRect.bottom);
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        ShowWindow(state.vmcSendPortEditWindow, SW_HIDE);
        return;
    }
    MoveWindow(state.vmcSendPortEditWindow, editRect.left, editRect.top,
        editRect.right - editRect.left, editRect.bottom - editRect.top, TRUE);
    ShowWindow(state.vmcSendPortEditWindow, SW_SHOW);
}

} // namespace ovtr::win32
