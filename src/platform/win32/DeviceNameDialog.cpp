#include "platform/win32/Dialogs.h"

#include "platform/win32/DeviceNameDialogConstants.h"
#include "platform/win32/DeviceNameDialogSession.h"

namespace ovtr::win32 {
namespace {

LRESULT CALLBACK deviceNameDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<DeviceNameDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<DeviceNameDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        createDeviceNameDialogControls(hwnd, *dialog);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == IDOK && dialog) {
            finishDeviceNameDialog(hwnd, *dialog, true);
            return 0;
        }
        if (LOWORD(wparam) == IDCANCEL && dialog) {
            finishDeviceNameDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (dialog) {
            finishDeviceNameDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

bool registerDeviceNameDialogClass(HINSTANCE instance)
{
    WNDCLASSW dialogClass{};
    dialogClass.style = CS_HREDRAW | CS_VREDRAW;
    dialogClass.lpfnWndProc = deviceNameDialogProc;
    dialogClass.hInstance = instance;
    dialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    dialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    dialogClass.lpszClassName = kDeviceNameDialogClassName;

    return RegisterClassW(&dialogClass) != 0;
}

} // namespace ovtr::win32
