#include "platform/win32/Dialogs.h"

#include "platform/win32/RecordSettingsDialogConstants.h"
#include "platform/win32/RecordSettingsDialogControls.h"
#include "platform/win32/RecordSettingsDialogSession.h"

namespace ovtr::win32 {
namespace {

LRESULT CALLBACK recordSettingsDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<RecordSettingsDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<RecordSettingsDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        createRecordSettingsDialogControls(hwnd, dialog->result, dialog->controls);
        return 0;
    }
    case WM_COMMAND: {
        if (!dialog) {
            break;
        }

        const UINT command = LOWORD(wparam);
        if (command == kExportLocationBrowseControlId) {
            browseRecordSettingsDirectory(hwnd, *dialog);
            return 0;
        }
        if (command == kSessionLocationBrowseControlId) {
            browseRecordSettingsSessionDirectory(hwnd, *dialog);
            return 0;
        }
        if (command == IDOK) {
            finishRecordSettingsDialog(hwnd, *dialog, true);
            return 0;
        }
        if (command == IDCANCEL) {
            finishRecordSettingsDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (dialog) {
            finishRecordSettingsDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

bool registerRecordSettingsDialogClass(HINSTANCE instance)
{
    WNDCLASSW dialogClass{};
    dialogClass.style = CS_HREDRAW | CS_VREDRAW;
    dialogClass.lpfnWndProc = recordSettingsDialogProc;
    dialogClass.hInstance = instance;
    dialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    dialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    dialogClass.lpszClassName = kRecordSettingsDialogClassName;

    return RegisterClassW(&dialogClass) != 0;
}

} // namespace ovtr::win32
