#include "platform/win32/OriginDialogControls.h"

#include "platform/win32/OriginDialogControlSections.h"

namespace ovtr::win32 {

void createOriginDialogControls(HWND hwnd, OriginDialogControls& controls)
{
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    controls.enabledCheck = createOriginDialogEnabledCheckbox(hwnd, font);
    createOriginDialogAxisHeaders(hwnd, font);
    createOriginDialogValueRows(hwnd, font, controls);
    createOriginDialogActionButtons(hwnd, font);
    focusFirstOriginDialogEdit(controls);
}

} // namespace ovtr::win32
