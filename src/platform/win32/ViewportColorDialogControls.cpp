#include "platform/win32/ViewportColorDialogControls.h"

#include "platform/win32/ViewportColorDialogControlSections.h"

namespace ovtr::win32 {

void createViewportColorDialogControls(HWND hwnd, ViewportColorDialogControls& controls)
{
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    createViewportColorHeaderControls(hwnd, font);
    createViewportColorRows(hwnd, font, controls);
    createViewportColorFooterControls(hwnd, font, controls);
}

} // namespace ovtr::win32
