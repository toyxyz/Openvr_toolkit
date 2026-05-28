#include "platform/win32/DeviceNameDialogSession.h"

#include "platform/win32/Dialogs.h"

namespace ovtr::win32 {

void finishDeviceNameDialog(HWND hwnd, DeviceNameDialogState& dialog, const bool accepted)
{
    dialog.accepted = accepted;
    if (accepted && dialog.editWindow) {
        dialog.resultName = readTrimmedWindowText(dialog.editWindow);
    }

    dialog.done = true;
    DestroyWindow(hwnd);
}

} // namespace ovtr::win32
