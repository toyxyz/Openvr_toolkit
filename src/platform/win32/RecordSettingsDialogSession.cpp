#include "platform/win32/RecordSettingsDialogSession.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/Win32String.h"

#include "export/ExportTypes.h"

namespace ovtr::win32 {
namespace {

float cutoffFromCombo(HWND combo)
{
    constexpr float options[] = {0.5f, 1.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 15.0f, 20.0f};
    const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (selected < 0 || selected >= 9) {
        return 8.0f;
    }
    return options[static_cast<int>(selected)];
}

OutlierRepairStrength outlierRepairStrengthFromCombo(HWND combo)
{
    const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    switch (selected) {
    case 0:
        return OutlierRepairStrength::None;
    case 2:
        return OutlierRepairStrength::Normal;
    case 3:
        return OutlierRepairStrength::Strong;
    case 1:
    default:
        return OutlierRepairStrength::Light;
    }
}

} // namespace

void browseRecordSettingsDirectory(HWND hwnd, RecordSettingsDialogState& dialog)
{
    std::filesystem::path selectedDirectory;
    if (chooseExportDirectory(
            hwnd,
            normalizedExportDirectoryPath(dialog.result.directory),
            selectedDirectory
        )) {
        dialog.result.directory = normalizedExportDirectoryPath(selectedDirectory);
        setEditText(dialog.controls.directoryEdit, dialog.result.directory.wstring());
    }
}

void finishRecordSettingsDialog(HWND hwnd, RecordSettingsDialogState& dialog, const bool accepted)
{
    if (accepted) {
        if (!dialog.controls.directoryEdit ||
            !dialog.controls.delayEdit ||
            !dialog.controls.resampleFpsEdit ||
            !dialog.controls.startRecordingOnCalibrationCheck ||
            !dialog.controls.exportAfterRecordingCheck ||
            !dialog.controls.applyNoiseFilterOnExportCheck ||
            !dialog.controls.noiseFilterCutoffCombo ||
            !dialog.controls.outlierRepairStrengthCombo ||
            !dialog.controls.smoothingIterationsEdit) {
            return;
        }

        const std::filesystem::path directory = normalizedExportDirectoryPath(
            std::filesystem::path(readTrimmedWindowText(dialog.controls.directoryEdit))
        );
        std::error_code createError;
        std::filesystem::create_directories(directory, createError);
        if (createError) {
            const std::wstring message = L"Could not create export folder:\n" +
                directory.wstring() + L"\n\n" + widen(createError.message());
            MessageBoxW(hwnd, message.c_str(), L"Record Settings", MB_OK | MB_ICONWARNING);
            return;
        }

        float recordDelaySeconds = 0.0f;
        if (!readFiniteFloatEdit(dialog.controls.delayEdit, recordDelaySeconds) ||
            recordDelaySeconds < 0.0f) {
            MessageBoxW(
                hwnd,
                L"Record delay must be 0 or greater.",
                L"Record Settings",
                MB_OK | MB_ICONWARNING
            );
            return;
        }

        float exportSampleRate = dialog.input.defaultExportSampleRate;
        if (!readFiniteFloatEdit(dialog.controls.resampleFpsEdit, exportSampleRate) ||
            exportSampleRate <= 0.0f ||
            exportSampleRate > static_cast<float>(ovtr::kMaxExportSampleRate)) {
            MessageBoxW(
                hwnd,
                L"Resample FPS must be greater than 0 and no more than 1000.",
                L"Record Settings",
                MB_OK | MB_ICONWARNING
            );
            return;
        }

        const bool startRecordingOnCalibration = SendMessageW(
            dialog.controls.startRecordingOnCalibrationCheck,
            BM_GETCHECK,
            0,
            0
        ) == BST_CHECKED;
        const bool exportAfterRecording = SendMessageW(
            dialog.controls.exportAfterRecordingCheck,
            BM_GETCHECK,
            0,
            0
        ) == BST_CHECKED;
        const bool applyNoiseFilterOnExport = SendMessageW(
            dialog.controls.applyNoiseFilterOnExportCheck,
            BM_GETCHECK,
            0,
            0
        ) == BST_CHECKED;
        int smoothingIterations = 0;
        if (!readIntegerEdit(dialog.controls.smoothingIterationsEdit, smoothingIterations) ||
            smoothingIterations < 0 ||
            smoothingIterations > 100) {
            MessageBoxW(
                hwnd,
                L"Smoothing iterations must be from 0 to 100.",
                L"Record Settings",
                MB_OK | MB_ICONWARNING
            );
            return;
        }
        dialog.result = sanitizedRecordSettingsDialogResult(
            directory,
            recordDelaySeconds,
            exportSampleRate,
            startRecordingOnCalibration,
            exportAfterRecording,
            applyNoiseFilterOnExport,
            cutoffFromCombo(dialog.controls.noiseFilterCutoffCombo),
            outlierRepairStrengthFromCombo(dialog.controls.outlierRepairStrengthCombo),
            smoothingIterations,
            dialog.input.defaultExportSampleRate
        );
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

} // namespace ovtr::win32
