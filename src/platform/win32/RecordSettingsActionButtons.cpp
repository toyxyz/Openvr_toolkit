#include "platform/win32/RecordSettingsDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createRecordSettingsActionButtons(HWND hwnd, HFONT font)
{
    HWND okButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        370,
        198,
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
        462,
        198,
        78,
        26,
        hwnd,
        reinterpret_cast<HMENU>(IDCANCEL),
        nullptr,
        nullptr
    );

    applyControlFont(okButton, font);
    applyControlFont(cancelButton, font);
}

} // namespace ovtr::win32
