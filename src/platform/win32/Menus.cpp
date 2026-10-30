#include "platform/win32/Menus.h"
#include "platform/win32/UiTheme.h"
#include "platform/win32/WindowPaintFonts.h"

#include "platform/win32/Win32GdiResources.h"
#include "platform/win32/Win32ScopedDcResources.h"

#include <algorithm>

namespace ovtr::win32 {

void appendPopupMenuItem(HMENU menu, PopupMenuItem& item)
{
    static const UniqueBrush menuBackground(CreateSolidBrush(ui::panel));
    MENUINFO appearance{};
    appearance.cbSize = sizeof(appearance);
    appearance.fMask = MIM_BACKGROUND;
    appearance.hbrBack = menuBackground.get();
    if (!SetMenuInfo(menu, &appearance)) {
        OutputDebugStringW(L"UI theme: setting popup menu background failed.\n");
    }
    AppendMenuW(
        menu,
        MF_OWNERDRAW,
        item.commandId,
        reinterpret_cast<LPCWSTR>(&item)
    );
}

bool measurePopupMenuItem(HWND hwnd, MEASUREITEMSTRUCT& measure)
{
    if (measure.CtlType != ODT_MENU || measure.itemData == 0) {
        return false;
    }

    const auto* item = reinterpret_cast<const PopupMenuItem*>(measure.itemData);
    WindowDc windowDc(hwnd);
    HDC dc = windowDc.get();
    const auto fonts = createWindowPaintFonts();
    HFONT font = fonts.bodyFont();
    SelectObjectGuard fontSelection(dc, font);

    SIZE textSize{96, 18};
    if (dc) {
        GetTextExtentPoint32W(
            dc,
            item->label.c_str(),
            static_cast<int>(item->label.size()),
            &textSize
        );
    }

    measure.itemWidth = static_cast<UINT>(std::max<LONG>(textSize.cx + 72, 168));
    measure.itemHeight = 26;
    return true;
}

bool drawPopupMenuItem(const DRAWITEMSTRUCT& draw)
{
    if (draw.CtlType != ODT_MENU || draw.itemData == 0 || !draw.hDC) {
        return false;
    }

    const auto* item = reinterpret_cast<const PopupMenuItem*>(draw.itemData);
    const bool selected = (draw.itemState & ODS_SELECTED) != 0;
    const bool disabled = (draw.itemState & ODS_DISABLED) != 0;

    UniqueBrush backgroundBrush(CreateSolidBrush(selected ? ui::accent : ui::panel));
    FillRect(draw.hDC, &draw.rcItem, backgroundBrush.get());

    if (selected) {
        UniquePen borderPen(CreatePen(PS_SOLID, 1, ui::accent));
        SelectObjectGuard penSelection(draw.hDC, borderPen.get());
        SelectObjectGuard brushSelection(draw.hDC, GetStockObject(NULL_BRUSH));
        RoundRect(
            draw.hDC,
            draw.rcItem.left + 4,
            draw.rcItem.top + 3,
            draw.rcItem.right - 4,
            draw.rcItem.bottom - 3,
            5,
            5
        );
    }

    RECT textRect = draw.rcItem;
    textRect.left += 28;
    textRect.right -= 18;
    SetBkMode(draw.hDC, TRANSPARENT);
    SetTextColor(draw.hDC, disabled ? ui::disabled : ui::text);

    const auto fonts = createWindowPaintFonts();
    HFONT font = fonts.bodyFont();
    SelectObjectGuard fontSelection(draw.hDC, font);
    DrawTextW(
        draw.hDC,
        item->label.c_str(),
        static_cast<int>(item->label.size()),
        &textRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX
    );
    return true;
}

} // namespace ovtr::win32
