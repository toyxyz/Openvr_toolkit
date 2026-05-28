#include "platform/win32/DeviceNameDialogSession.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createDeviceNameDialogControls(HWND hwnd, DeviceNameDialogState& dialog)
{
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HWND deviceLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Device",
        WS_CHILD | WS_VISIBLE,
        16,
        14,
        388,
        18,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND deviceValue = CreateWindowExW(
        0,
        L"STATIC",
        dialog.deviceLabel.c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        16,
        36,
        398,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND nameLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Name",
        WS_CHILD | WS_VISIBLE,
        16,
        68,
        388,
        18,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    dialog.editWindow = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        dialog.initialName.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        16,
        90,
        398,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kDeviceNameEditControlId),
        nullptr,
        nullptr
    );
    HWND okButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        248,
        126,
        78,
        26,
        hwnd,
        reinterpret_cast<HMENU>(IDOK),
        nullptr,
        nullptr
    );
    HWND cancelButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        336,
        126,
        78,
        26,
        hwnd,
        reinterpret_cast<HMENU>(IDCANCEL),
        nullptr,
        nullptr
    );

    HWND controls[] = {deviceLabel, deviceValue, nameLabel, dialog.editWindow, okButton, cancelButton};
    for (HWND child : controls) {
        applyControlFont(child, font);
    }
    focusAndSelectAllText(dialog.editWindow);
}

} // namespace ovtr::win32
