#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/RecordSettingsDialogControls.h"

namespace ovtr::win32 {

void createRecordSettingsExportFolderControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
);
void createRecordSettingsCaptureControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
);
void createRecordSettingsActionButtons(HWND hwnd, HFONT font);

} // namespace ovtr::win32
