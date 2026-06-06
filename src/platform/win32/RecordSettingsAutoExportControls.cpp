#include "platform/win32/RecordSettingsCaptureControlParts.h"

#include "platform/win32/DialogControlHelpers.h"

namespace ovtr::win32 {

void createExportAfterRecordingControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    controls.exportAfterRecordingCheck = CreateWindowExW(
        0,
        L"BUTTON",
        L"Export after recording",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        300,
        208,
        230,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kExportAfterRecordingControlId),
        nullptr,
        nullptr
    );
    if (controls.exportAfterRecordingCheck) {
        SendMessageW(
            controls.exportAfterRecordingCheck,
            BM_SETCHECK,
            result.exportAfterRecording ? BST_CHECKED : BST_UNCHECKED,
            0
        );
    }
    applyControlFont(controls.exportAfterRecordingCheck, font);
}

} // namespace ovtr::win32
