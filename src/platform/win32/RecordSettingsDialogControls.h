#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/RecordSettingsModel.h"

namespace ovtr::win32 {

inline constexpr UINT_PTR kExportLocationEditControlId = 4500;
inline constexpr UINT_PTR kExportLocationBrowseControlId = 4501;
inline constexpr UINT_PTR kRecordDelayEditControlId = 4502;
inline constexpr UINT_PTR kRecordSaveFormatControlId = 4503;
inline constexpr UINT_PTR kRecordResampleFpsEditControlId = 4504;

struct RecordSettingsDialogControls {
    HWND directoryEdit = nullptr;
    HWND delayEdit = nullptr;
    HWND resampleFpsEdit = nullptr;
    HWND saveFormatCombo = nullptr;
};

void createRecordSettingsDialogControls(
    HWND hwnd,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
);

} // namespace ovtr::win32
