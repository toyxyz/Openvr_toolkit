#include "platform/win32/RecordSettingsCaptureControlParts.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/DialogControlHelpers.h"
#include "platform/win32/Dialogs.h"

namespace ovtr::win32 {
namespace {

constexpr float kNoiseFilterCutoffOptions[] = {0.5f, 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 15.0f, 20.0f};

int selectedNoiseFilterCutoffIndex(const float cutoffHz) noexcept
{
    const float sanitized = sanitizedNoiseFilterCutoffHz(cutoffHz);
    for (int index = 0; index < 9; ++index) {
        if (kNoiseFilterCutoffOptions[index] == sanitized) {
            return index;
        }
    }
    return 3;
}

int selectedOutlierRepairStrengthIndex(const OutlierRepairStrength strength) noexcept
{
    switch (strength) {
    case OutlierRepairStrength::None:
        return 0;
    case OutlierRepairStrength::Normal:
        return 2;
    case OutlierRepairStrength::Strong:
        return 3;
    case OutlierRepairStrength::Light:
    default:
        return 1;
    }
}

} // namespace

void createRecordResampleControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    HWND resampleFpsLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Resample FPS",
        WS_CHILD | WS_VISIBLE,
        18,
        204,
        140,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.resampleFpsEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        formatFloatText(result.exportSampleRate).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        18,
        230,
        96,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kRecordResampleFpsEditControlId),
        nullptr,
        nullptr
    );
    HWND resampleFpsHint = CreateWindowExW(
        0,
        L"STATIC",
        L"fps",
        WS_CHILD | WS_VISIBLE,
        126,
        233,
        42,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );

    applyControlFont(resampleFpsLabel, font);
    applyControlFont(controls.resampleFpsEdit, font);
    applyControlFont(resampleFpsHint, font);
}

void createStartRecordingOnCalibrationControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    controls.startRecordingOnCalibrationCheck = CreateWindowExW(
        0,
        L"BUTTON",
        L"Start recording on calibration",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        18,
        266,
        260,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kStartRecordingOnCalibrationControlId),
        nullptr,
        nullptr
    );
    if (controls.startRecordingOnCalibrationCheck) {
        SendMessageW(
            controls.startRecordingOnCalibrationCheck,
            BM_SETCHECK,
            result.startRecordingOnCalibration ? BST_CHECKED : BST_UNCHECKED,
            0
        );
    }
    applyControlFont(controls.startRecordingOnCalibrationCheck, font);
}

void createNoiseFilterOnExportControls(
    HWND hwnd,
    HFONT font,
    const RecordSettingsDialogResult& result,
    RecordSettingsDialogControls& controls
)
{
    controls.applyNoiseFilterOnExportCheck = CreateWindowExW(
        0,
        L"BUTTON",
        L"Apply noise filter on export",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        18,
        298,
        300,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kApplyNoiseFilterOnExportControlId),
        nullptr,
        nullptr
    );
    if (controls.applyNoiseFilterOnExportCheck) {
        SendMessageW(
            controls.applyNoiseFilterOnExportCheck,
            BM_SETCHECK,
            result.applyNoiseFilterOnExport ? BST_CHECKED : BST_UNCHECKED,
            0
        );
    }

    HWND cutoffLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Cutoff Hz",
        WS_CHILD | WS_VISIBLE,
        18,
        334,
        80,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.noiseFilterCutoffCombo = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        108,
        330,
        80,
        140,
        hwnd,
        reinterpret_cast<HMENU>(kNoiseFilterCutoffComboControlId),
        nullptr,
        nullptr
    );
    constexpr const wchar_t* labels[] = {L"0.5", L"1", L"2", L"4", L"6", L"8", L"10", L"15", L"20"};
    for (const wchar_t* label : labels) {
        SendMessageW(controls.noiseFilterCutoffCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }
    SendMessageW(
        controls.noiseFilterCutoffCombo,
        CB_SETCURSEL,
        static_cast<WPARAM>(selectedNoiseFilterCutoffIndex(result.noiseFilterCutoffHz)),
        0
    );

    HWND repairLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Outlier repair",
        WS_CHILD | WS_VISIBLE,
        18,
        366,
        96,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.outlierRepairStrengthCombo = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        124,
        362,
        96,
        120,
        hwnd,
        reinterpret_cast<HMENU>(kOutlierRepairStrengthComboControlId),
        nullptr,
        nullptr
    );
    constexpr const wchar_t* repairLabels[] = {L"None", L"Light", L"Normal", L"Strong"};
    for (const wchar_t* label : repairLabels) {
        SendMessageW(controls.outlierRepairStrengthCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }
    SendMessageW(
        controls.outlierRepairStrengthCombo,
        CB_SETCURSEL,
        static_cast<WPARAM>(selectedOutlierRepairStrengthIndex(result.outlierRepairStrength)),
        0
    );

    HWND smoothingLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Smoothing iters",
        WS_CHILD | WS_VISIBLE,
        18,
        398,
        112,
        20,
        hwnd,
        nullptr,
        nullptr,
        nullptr
    );
    controls.smoothingIterationsEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        formatIntegerText(result.smoothingIterations).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        136,
        394,
        64,
        24,
        hwnd,
        reinterpret_cast<HMENU>(kSmoothingIterationsEditControlId),
        nullptr,
        nullptr
    );

    applyControlFont(controls.applyNoiseFilterOnExportCheck, font);
    applyControlFont(cutoffLabel, font);
    applyControlFont(controls.noiseFilterCutoffCombo, font);
    applyControlFont(repairLabel, font);
    applyControlFont(controls.outlierRepairStrengthCombo, font);
    applyControlFont(smoothingLabel, font);
    applyControlFont(controls.smoothingIterationsEdit, font);
}

} // namespace ovtr::win32
