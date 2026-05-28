#include "platform/win32/OriginDialog.h"

#include "platform/win32/OriginDialogConstants.h"
#include "platform/win32/OriginDialogModel.h"
#include "platform/win32/OriginDialogSession.h"
#include "platform/win32/ModalDialog.h"

namespace ovtr::win32 {

bool showOriginSettings(HWND parent, AppWindowState& state)
{
    OriginDialogState dialog;
    dialog.parent = parent;
    dialog.appState = &state;
    dialog.originalValues = originDialogValuesFromState(state);
    dialog.workingValues = dialog.originalValues;

    ModalDialogHost host(parent);
    const ModalDialogDescriptor descriptor{
        kOriginDialogClassName,
        L"Origin Settings",
        kOriginDialogWidth,
        kOriginDialogHeight,
    };
    HWND dialogWindow = host.create(descriptor, &dialog);
    if (!dialogWindow) {
        return false;
    }

    host.runMessageLoop(dialogWindow, dialog.done);

    if (!dialog.done) {
        restoreOriginDialogSnapshot(dialogWindow, dialog);
    }

    host.restoreParent();
    host.repostQuitIfReceived();
    return dialog.accepted;
}

} // namespace ovtr::win32
