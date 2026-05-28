#include "platform/win32/Dialogs.h"

#include "platform/win32/DeviceNameDialogConstants.h"
#include "platform/win32/DeviceNameDialogSession.h"
#include "platform/win32/ModalDialog.h"

namespace ovtr::win32 {
namespace {

bool promptForName(
    HWND parent,
    const std::wstring& title,
    const std::wstring& subjectLabel,
    const std::wstring& deviceLabel,
    const std::wstring& initialName,
    std::wstring& outName
)
{
    DeviceNameDialogState dialog;
    dialog.subjectLabel = subjectLabel;
    dialog.deviceLabel = deviceLabel;
    dialog.initialName = initialName;
    dialog.resultName = dialog.initialName;

    ModalDialogHost host(parent);
    const ModalDialogDescriptor descriptor{
        kDeviceNameDialogClassName,
        title.c_str(),
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

} // namespace

bool promptForDeviceName(
    HWND parent,
    const std::wstring& deviceLabel,
    const std::wstring& initialName,
    std::wstring& outName
)
{
    return promptForName(parent, L"Set Name", L"Device", deviceLabel, initialName, outName);
}

bool promptForMarkerName(
    HWND parent,
    const std::wstring& markerLabel,
    const std::wstring& initialName,
    std::wstring& outName
)
{
    return promptForName(parent, L"Rename Marker", L"Marker", markerLabel, initialName, outName);
}

} // namespace ovtr::win32
