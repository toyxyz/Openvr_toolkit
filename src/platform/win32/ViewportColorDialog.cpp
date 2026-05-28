#include "platform/win32/Dialogs.h"

#include "platform/win32/ViewportColorDialogConstants.h"
#include "platform/win32/ViewportColorDialogControls.h"
#include "platform/win32/ViewportColorDialogSession.h"
#include "platform/win32/ViewportColorDialogSessionInternal.h"

namespace ovtr::win32 {
namespace {

bool isColorEditChangeCommand(const UINT command, const UINT notification) noexcept
{
    if (notification != EN_CHANGE ||
        command < kViewportColorEditBaseControlId ||
        command >= kViewportColorEditBaseControlId + kViewportColorSlotCount * 10) {
        return false;
    }

    return (command - kViewportColorEditBaseControlId) % 10 < 3;
}

void invalidateColorEditSwatch(const ViewportColorDialogState& dialog, const UINT command) noexcept
{
    const UINT index = (command - kViewportColorEditBaseControlId) / 10;
    if (index < kViewportColorSlotCount) {
        InvalidateRect(dialog.controls.colors[index].swatch, nullptr, TRUE);
    }
}

LRESULT CALLBACK viewportColorDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* dialog = reinterpret_cast<ViewportColorDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE: {
        const auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lparam);
        dialog = reinterpret_cast<ViewportColorDialogState*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        return TRUE;
    }
    case WM_CREATE: {
        if (!dialog) {
            return -1;
        }

        createViewportColorDialogControls(hwnd, dialog->controls);
        updateViewportColorDialogControls(*dialog);
        return 0;
    }
    case WM_COMMAND: {
        if (!dialog) {
            break;
        }
        const UINT command = LOWORD(wparam);
        const UINT notification = HIWORD(wparam);
        if (isColorEditChangeCommand(command, notification)) {
            invalidateColorEditSwatch(*dialog, command);
            return 0;
        }
        if (command >= kViewportColorPickBaseControlId &&
            command < kViewportColorPickBaseControlId + kViewportColorSlotCount) {
            chooseViewportDialogColor(hwnd, *dialog, static_cast<int>(command - kViewportColorPickBaseControlId));
            return 0;
        }
        if (command == kViewportColorResetControlId) {
            resetViewportDialogColors(*dialog);
            return 0;
        }
        if (command == IDOK) {
            finishViewportColorDialog(hwnd, *dialog, true);
            return 0;
        }
        if (command == IDCANCEL) {
            finishViewportColorDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    }
    case WM_DRAWITEM:
        if (dialog) {
            const auto* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lparam);
            if (item && drawViewportColorSwatch(*dialog, *item)) {
                return TRUE;
            }
        }
        break;
    case WM_CLOSE:
        if (dialog) {
            finishViewportColorDialog(hwnd, *dialog, false);
            return 0;
        }
        break;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

bool registerViewportColorDialogClass(HINSTANCE instance)
{
    WNDCLASSW dialogClass{};
    dialogClass.style = CS_HREDRAW | CS_VREDRAW;
    dialogClass.lpfnWndProc = viewportColorDialogProc;
    dialogClass.hInstance = instance;
    dialogClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    dialogClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    dialogClass.lpszClassName = kViewportColorDialogClassName;

    return RegisterClassW(&dialogClass) != 0;
}

} // namespace ovtr::win32
