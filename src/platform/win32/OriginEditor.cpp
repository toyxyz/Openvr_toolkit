#include "platform/win32/OriginEditor.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/OriginEditorSubclass.h"
#include "platform/win32/OriginState.h"
#include "platform/win32/WindowLayout.h"

namespace ovtr::win32 {
namespace {

constexpr UINT_PTR kOriginEditControlId = 2001;

} // namespace

void closeOriginEditor(HWND hwnd, AppWindowState& state)
{
    HWND editWindow = state.originEditWindow;
    if (!editWindow) {
        return;
    }

    if (state.originEditOriginalProc) {
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(state.originEditOriginalProc));
    }
    state.originEditWindow = nullptr;
    state.originEditOriginalProc = nullptr;
    DestroyWindow(editWindow);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void showOriginEditor(HWND hwnd, AppWindowState& state)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    const RECT editRect = originEditorRectForClient(
        &state,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    if (editRect.right <= editRect.left || editRect.bottom <= editRect.top) {
        return;
    }

    if (state.originEditWindow && IsWindow(state.originEditWindow)) {
        SetWindowTextW(state.originEditWindow, formatOriginEditorText(state).c_str());
        MoveWindow(
            state.originEditWindow,
            editRect.left,
            editRect.top,
            editRect.right - editRect.left,
            editRect.bottom - editRect.top,
            TRUE
        );
        ShowWindow(state.originEditWindow, SW_SHOW);
        focusAndSelectAllText(state.originEditWindow);
        return;
    }

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    state.originEditWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        formatOriginEditorText(state).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        editRect.left,
        editRect.top,
        editRect.right - editRect.left,
        editRect.bottom - editRect.top,
        hwnd,
        reinterpret_cast<HMENU>(kOriginEditControlId),
        instance,
        nullptr
    );
    if (!state.originEditWindow) {
        return;
    }

    SendMessageW(
        state.originEditWindow,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
        TRUE
    );
    state.originEditOriginalProc = installOriginEditorSubclass(state.originEditWindow);
    focusAndSelectAllText(state.originEditWindow);
    appendDebugLog(state, L"Origin editor opened");
    InvalidateRect(hwnd, &editRect, FALSE);
}

} // namespace ovtr::win32
