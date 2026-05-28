#include "platform/win32/OriginDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

HWND createOriginDialogEnabledCheckbox(HWND hwnd, HFONT font)
{
    HWND enabledCheck = CreateWindowExW(
        0,
        L"BUTTON",
        L"Enable origin",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        22,
        18,
        180,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kOriginDialogEnabledControlId),
        nullptr,
        nullptr
    );
    applyControlFont(enabledCheck, font);
    return enabledCheck;
}

void createOriginDialogAxisHeaders(HWND hwnd, HFONT font)
{
    static constexpr const wchar_t* kAxisLabels[] = {L"X", L"Y", L"Z"};
    for (int axis = 0; axis < 3; ++axis) {
        const int x = 140 + axis * 92;
        HWND axisHeader = CreateWindowExW(
            0,
            L"STATIC",
            kAxisLabels[axis],
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            x,
            54,
            72,
            18,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        applyControlFont(axisHeader, font);
    }
}

} // namespace ovtr::win32
