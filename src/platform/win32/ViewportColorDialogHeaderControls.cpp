#include "platform/win32/ViewportColorDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createViewportColorHeaderControls(HWND hwnd, HFONT font)
{
    struct HeaderLabel {
        const wchar_t* text;
        RECT rect;
    };
    static constexpr HeaderLabel kHeaderLabels[] = {
        {L"R", RECT{214, 14, 262, 32}},
        {L"G", RECT{274, 14, 322, 32}},
        {L"B", RECT{334, 14, 382, 32}},
        {L"Preview", RECT{486, 14, 544, 32}},
    };

    for (const HeaderLabel& header : kHeaderLabels) {
        HWND headerWindow = CreateWindowExW(
            0,
            L"STATIC",
            header.text,
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            header.rect.left,
            header.rect.top,
            header.rect.right - header.rect.left,
            header.rect.bottom - header.rect.top,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        applyControlFont(headerWindow, font);
    }
}

} // namespace ovtr::win32
