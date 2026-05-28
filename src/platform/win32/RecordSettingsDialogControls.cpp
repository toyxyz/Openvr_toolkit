#include "platform/win32/RecordSettingsDialogControls.h"

#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/RecordSettingsDialogControlSections.h"

namespace ovtr::win32 {

void createRecordSettingsDialogControls(
    HWND hwnd,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    createRecordSettingsExportFolderControls(hwnd, font, result, controls);
    createRecordSettingsCaptureControls(hwnd, font, result, controls);
    createRecordSettingsActionButtons(hwnd, font);
    focusAndSelectAllText(controls.directoryEdit);
}

} // namespace ovtr::win32
