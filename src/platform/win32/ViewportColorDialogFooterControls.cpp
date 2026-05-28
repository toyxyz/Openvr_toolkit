#include "platform/win32/ViewportColorDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createViewportColorFooterControls(HWND hwnd, HFONT font, ViewportColorDialogControls& controls)
{
    HWND outlineLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Device outline thickness",
        WS_CHILD | WS_VISIBLE,
        18,
        270,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.outlineEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        210,
        266,
        88,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kViewportOutlineEditControlId),
        nullptr,
        nullptr
    );
    HWND outlineHint = CreateWindowExW(
        0,
        L"STATIC",
        L"Multiplier, 0.0 - 10.0",
        WS_CHILD | WS_VISIBLE,
        312,
        270,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND gridSizeLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Grid size",
        WS_CHILD | WS_VISIBLE,
        18,
        306,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.gridSizeEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        210,
        302,
        88,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kViewportGridSizeEditControlId),
        nullptr,
        nullptr
    );
    HWND gridSizeHint = CreateWindowExW(
        0,
        L"STATIC",
        L"Half size, 1.0 - 50.0",
        WS_CHILD | WS_VISIBLE,
        312,
        306,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND gridDensityLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Grid cell density",
        WS_CHILD | WS_VISIBLE,
        18,
        342,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.gridDensityEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        210,
        338,
        88,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kViewportGridDensityEditControlId),
        nullptr,
        nullptr
    );
    HWND gridDensityHint = CreateWindowExW(
        0,
        L"STATIC",
        L"Cells/unit, 0.25 - 10.0",
        WS_CHILD | WS_VISIBLE,
        312,
        342,
        180,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND resetButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Reset",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        270,
        398,
        78,
        26,
        hwnd,
        reinterpret_cast<HMENU>(kViewportColorResetControlId),
        nullptr,
        nullptr
    );
    HWND okButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        360,
        398,
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
        450,
        398,
        78,
        26,
        hwnd,
        reinterpret_cast<HMENU>(IDCANCEL),
        nullptr,
        nullptr
    );

    HWND footerControls[] = {
        outlineLabel, controls.outlineEdit, outlineHint,
        gridSizeLabel, controls.gridSizeEdit, gridSizeHint,
        gridDensityLabel, controls.gridDensityEdit, gridDensityHint,
        resetButton, okButton, cancelButton
    };
    for (HWND child : footerControls) {
        applyControlFont(child, font);
    }
}

} // namespace ovtr::win32
