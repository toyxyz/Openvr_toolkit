#include "platform/win32/Dialogs.h"

#include "platform/win32/DeviceNameDialogConstants.h"
#include "platform/win32/DeviceNameDialogSession.h"
#include "platform/win32/ModalDialog.h"

namespace ovtr::win32 {

bool promptForDeviceName(
    HWND parent,
    const std::wstring& deviceLabel,
    const std::wstring& initialName,
    std::wstring& outName
)
{
    DeviceNameDialogState dialog;
    dialog.deviceLabel = deviceLabel;
    dialog.initialName = initialName;
    dialog.resultName = dialog.initialName;

    ModalDialogHost host(parent);
    const ModalDialogDescriptor descriptor{
        kDeviceNameDialogClassName,
        L"Set Name",
        kDeviceNameDialogWidth,
        kDeviceNameDialogHeight,
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

    outName = dialog.resultName;
    return true;
}

} // namespace ovtr::win32
