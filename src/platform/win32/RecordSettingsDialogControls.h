#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "platform/win32/RecordSettingsModel.h"

namespace ovtr::win32 {

inline constexpr UINT_PTR kExportLocationEditControlId = 4500;
inline constexpr UINT_PTR kExportLocationBrowseControlId = 4501;
inline constexpr UINT_PTR kSessionLocationEditControlId = 4511;
inline constexpr UINT_PTR kSessionLocationBrowseControlId = 4512;
inline constexpr UINT_PTR kRecordDelayEditControlId = 4502;
inline constexpr UINT_PTR kRecordResampleFpsEditControlId = 4504;
inline constexpr UINT_PTR kStartRecordingOnCalibrationControlId = 4505;
inline constexpr UINT_PTR kApplyNoiseFilterOnExportControlId = 4506;
inline constexpr UINT_PTR kNoiseFilterCutoffComboControlId = 4507;
inline constexpr UINT_PTR kOutlierRepairStrengthComboControlId = 4508;
inline constexpr UINT_PTR kSmoothingIterationsEditControlId = 4509;
inline constexpr UINT_PTR kExportAfterRecordingControlId = 4510;

struct RecordSettingsDialogControls {
    HWND directoryEdit = nullptr;
    HWND sessionDirectoryEdit = nullptr;
    HWND delayEdit = nullptr;
    HWND resampleFpsEdit = nullptr;
    HWND startRecordingOnCalibrationCheck = nullptr;
    HWND exportAfterRecordingCheck = nullptr;
    HWND applyNoiseFilterOnExportCheck = nullptr;
    HWND noiseFilterCutoffCombo = nullptr;
    HWND outlierRepairStrengthCombo = nullptr;
    HWND smoothingIterationsEdit = nullptr;
};

void createRecordSettingsDialogControls(
    HWND hwnd,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
);

} // namespace ovtr::win32
