#include "platform/win32/Dialogs.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/ModalDialog.h"
#include "platform/win32/ViewportColorDialogConstants.h"
#include "platform/win32/ViewportColorDialogSession.h"

namespace ovtr::win32 {

bool promptForViewportColorSettings(
    HWND parent,
    ViewportSettings initialSettings,
    ViewportSettings& outSettings
)
{
    ViewportColorDialogState dialog;
    dialog.workingSettings = clampViewportSettings(initialSettings);
    dialog.customColors.fill(RGB(255, 255, 255));

    ModalDialogHost host(parent);
    const ModalDialogDescriptor descriptor{
        kViewportColorDialogClassName,
        L"Color Settings",
        kViewportColorDialogWidth,
        kViewportColorDialogHeight,
    };
    HWND dialogWindow = host.create(descriptor, &dialog);
    if (!dialogWindow) {
        return false;
    }

    host.runMessageLoop(dialogWindow, dialog.done);
    host.restoreParent();
    host.repostQuitIfReceived();

    if (!dialog.accepted) {
        return false;
    }

    outSettings = clampViewportSettings(dialog.workingSettings);
    return true;
}

} // namespace ovtr::win32
