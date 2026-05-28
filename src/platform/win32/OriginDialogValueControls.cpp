#include "platform/win32/OriginDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

#include <cstddef>

namespace ovtr::win32 {

void createOriginDialogValueRows(HWND hwnd, HFONT font, OriginDialogControls& controls)
{
    HWND positionLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Position",
        WS_CHILD | WS_VISIBLE,
        22,
        82,
        90,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    HWND rotationLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Rotation",
        WS_CHILD | WS_VISIBLE,
        22,
        126,
        90,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    applyControlFont(positionLabel, font);
    applyControlFont(rotationLabel, font);

    for (int axis = 0; axis < 3; ++axis) {
        const int x = 140 + axis * 92;
        controls.positionEdits[static_cast<std::size_t>(axis)] = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            x,
            78,
            72,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kOriginDialogEditBaseControlId + axis),
            nullptr,
            nullptr
        );
        controls.rotationEdits[static_cast<std::size_t>(axis)] = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            x,
            122,
            72,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kOriginDialogEditBaseControlId + 3 + axis),
            nullptr,
            nullptr
        );
        applyControlFont(controls.positionEdits[static_cast<std::size_t>(axis)], font);
        applyControlFont(controls.rotationEdits[static_cast<std::size_t>(axis)], font);
    }
}

} // namespace ovtr::win32
