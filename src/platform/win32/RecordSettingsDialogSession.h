#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/RecordSettingsDialogControls.h"
#include "platform/win32/RecordSettingsModel.h"

namespace ovtr::win32 {

struct RecordSettingsDialogState {
    RecordSettingsDialogControls controls;
    RecordSettingsDialogInput input;
    RecordSettingsDialogResult result;
    bool accepted = false;
    bool done = false;
};

void browseRecordSettingsDirectory(HWND hwnd, RecordSettingsDialogState& dialog);
void finishRecordSettingsDialog(HWND hwnd, RecordSettingsDialogState& dialog, bool accepted);

} // namespace ovtr::win32
