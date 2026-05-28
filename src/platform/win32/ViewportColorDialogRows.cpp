#include "platform/win32/ViewportColorDialogControlSections.h"
#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createViewportColorRows(HWND hwnd, HFONT font, ViewportColorDialogControls& controls)
{
    static constexpr const wchar_t* kColorLabels[] = {
        L"Device name font",
        L"Grid",
        L"Background",
        L"Imported GLB",
        L"Render model outline",
        L"Render model material",
    };

    for (int i = 0; i < kViewportColorSlotCount; ++i) {
        const int y = 38 + i * 38;
        HWND label = CreateWindowExW(
            0,
            L"STATIC",
            kColorLabels[i],
            WS_CHILD | WS_VISIBLE,
            18,
            y + 4,
            170,
            20,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        applyControlFont(label, font);

        ViewportColorEditControls colorControls;
        colorControls.red = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
            210,
            y,
            52,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kViewportColorEditBaseControlId + i * 10),
            nullptr,
            nullptr
        );
        colorControls.green = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
            270,
            y,
            52,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kViewportColorEditBaseControlId + i * 10 + 1),
            nullptr,
            nullptr
        );
        colorControls.blue = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
            330,
            y,
            52,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kViewportColorEditBaseControlId + i * 10 + 2),
            nullptr,
            nullptr
        );
        colorControls.pick = CreateWindowExW(
            0,
            L"BUTTON",
            L"Pick",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            404,
            y - 1,
            76,
            26,
            hwnd,
            reinterpret_cast<HMENU>(kViewportColorPickBaseControlId + i),
            nullptr,
            nullptr
        );
        colorControls.swatch = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
            498,
            y,
            30,
            24,
            hwnd,
            reinterpret_cast<HMENU>(kViewportColorSwatchBaseControlId + i),
            nullptr,
            nullptr
        );
        controls.colors[static_cast<std::size_t>(i)] = colorControls;

        HWND rowControls[] = {
            colorControls.red,
            colorControls.green,
            colorControls.blue,
            colorControls.pick,
            colorControls.swatch
        };
        for (HWND child : rowControls) {
            applyControlFont(child, font);
        }
    }
}

} // namespace ovtr::win32
