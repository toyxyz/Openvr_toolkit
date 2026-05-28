#include "platform/win32/RecordSettingsDialogSession.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/Dialogs.h"
#include "platform/win32/Win32String.h"

#include "export/ExportTypes.h"

namespace ovtr::win32 {

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
            !dialog.controls.saveFormatCombo) {
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

        const LRESULT saveFormatIndex = SendMessageW(dialog.controls.saveFormatCombo, CB_GETCURSEL, 0, 0);
        dialog.result = sanitizedRecordSettingsDialogResult(
            directory,
            recordDelaySeconds,
            exportSampleRate,
            saveFormatIndex == 1 ? ExportFormat::Fbx : ExportFormat::Glb,
            dialog.input.defaultExportSampleRate
        );
    }

    dialog.accepted = accepted;
    dialog.done = true;
    DestroyWindow(hwnd);
}

} // namespace ovtr::win32
