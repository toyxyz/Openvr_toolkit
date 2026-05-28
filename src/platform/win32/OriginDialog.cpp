#include "platform/win32/OriginDialog.h"

#include "platform/win32/OriginDialogConstants.h"
#include "platform/win32/OriginDialogControls.h"
#include "platform/win32/OriginDialogSession.h"

namespace ovtr::win32 {
namespace {

LRESULT CALLBACK originDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<OriginDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<OriginDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        createOriginDialogControls(hwnd, dialog->controls);
        updateOriginDialogControls(*dialog);
        return 0;
    }
    case WM_COMMAND: {
        if (!dialog) {
            break;
        }

        const UINT command = LOWORD(wparam);
        const UINT notification = HIWORD(wparam);
        if (command == kOriginDialogEnabledControlId && notification == BN_CLICKED) {
            previewOriginDialogFromControls(hwnd, *dialog);
            return 0;
        }
        if (command >= kOriginDialogEditBaseControlId &&
            command < kOriginDialogEditBaseControlId + 6 &&
            notification == EN_CHANGE) {
            previewOriginDialogFromControls(hwnd, *dialog);
            return 0;
        }
        if (command == IDOK) {
            finishOriginDialog(hwnd, *dialog, true);
            return 0;
        }
        if (command == IDCANCEL) {
            finishOriginDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (dialog) {
            finishOriginDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

bool registerOriginDialogClass(HINSTANCE instance)
{
    WNDCLASSW dialogClass{};
    dialogClass.style = CS_HREDRAW | CS_VREDRAW;
    dialogClass.lpfnWndProc = originDialogProc;
    dialogClass.hInstance = instance;
    dialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    dialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    dialogClass.lpszClassName = kOriginDialogClassName;

    return RegisterClassW(&dialogClass) != 0;
}

} // namespace ovtr::win32
