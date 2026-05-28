#include "platform/win32/OriginDialogSessionInternal.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"

namespace ovtr::win32 {

void finishOriginDialog(HWND hwnd, OriginDialogState& dialog, const bool accepted)
{
    if (accepted) {
        if (!readOriginDialogControls(hwnd, dialog, true)) {
            return;
        }
        if (dialog.appState) {
            renderOriginDialogPreview(hwnd, dialog);
            dialog.appState->originStatusMessage = dialog.appState->originEnabled
                ? "origin settings saved"
                : "origin settings disabled";
            appendDebugLog(*dialog.appState, dialog.appState->originStatusMessage);
            saveOriginConfig(*dialog.appState);
        }
    } else {
        restoreOriginDialogSnapshot(hwnd, dialog);
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

} // namespace ovtr::win32
