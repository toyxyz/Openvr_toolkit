#include "platform/win32/Dialogs.h"

#include "platform/win32/RecordSettingsDialogConstants.h"
#include "platform/win32/RecordSettingsDialogSession.h"
#include "platform/win32/RecordSettingsModel.h"
#include "platform/win32/ModalDialog.h"

namespace ovtr::win32 {

bool promptForRecordSettings(
    HWND parent,
    const RecordSettingsDialogInput& input,
    RecordSettingsDialogResult& outResult
)
{
    RecordSettingsDialogState dialog;
    dialog.input = input;
    dialog.result = initialRecordSettingsDialogResult(input);

    ModalDialogHost host(parent);
    const ModalDialogDescriptor descriptor{
        kRecordSettingsDialogClassName,
        L"Record Settings",
        kRecordSettingsDialogWidth,
        kRecordSettingsDialogHeight,
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

    outResult = dialog.result;
    return true;
}

} // namespace ovtr::win32
