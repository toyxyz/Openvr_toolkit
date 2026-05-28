#include "platform/win32/OriginEditorSubclass.h"

#include "platform/win32/AppState.h"
#include "platform/win32/OriginEditor.h"
#include "platform/win32/OriginEditorActions.h"
#include "platform/win32/WindowStateAccess.h"

namespace ovtr::win32 {
namespace {

LRESULT CALLBACK originEditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND parent = GetParent(hwnd);
    AppWindowState* state = parent ? appStateForWindow(parent) : nullptr;

    if (message == WM_KEYDOWN && state) {
        if (wparam == VK_RETURN) {
            applyOriginEditorText(parent, *state);
            return 0;
        }
        if (wparam == VK_ESCAPE) {
            closeOriginEditor(parent, *state);
            return 0;
        }
    }

    if (state && state->originEditOriginalProc) {
        return CallWindowProcW(state->originEditOriginalProc, hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

WNDPROC installOriginEditorSubclass(HWND editWindow)
{
    return reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(editWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originEditProc))
    );
}

} // namespace ovtr::win32
