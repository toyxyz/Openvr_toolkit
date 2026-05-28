#include "platform/win32/RecordSettingsDialogControlSections.h"

#include "platform/win32/RecordSettingsCaptureControlParts.h"

namespace ovtr::win32 {

void createRecordSettingsCaptureControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    createRecordDelayControls(hwnd, font, result, controls);
    createRecordSaveFormatControls(hwnd, font, result, controls);
    createRecordResampleControls(hwnd, font, result, controls);
}

} // namespace ovtr::win32
