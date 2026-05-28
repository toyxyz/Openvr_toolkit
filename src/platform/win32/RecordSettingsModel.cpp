#include "platform/win32/RecordSettingsModel.h"

#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

RecordSettingsDialogResult sanitizedRecordSettingsDialogResult(
    const std::filesystem::path& directory,
    const float recordDelaySeconds,
    const float exportSampleRate,
    const ExportFormat saveFormat,
    const float defaultExportSampleRate
)
{
    RecordSettingsDialogResult result;
    result.directory = normalizedExportDirectoryPath(directory);
    result.recordDelaySeconds = sanitizedRecordDelaySeconds(recordDelaySeconds);
    result.exportSampleRate = sanitizedExportSampleRate(exportSampleRate, defaultExportSampleRate);
    result.saveFormat = saveFormat;
    return result;
}

RecordSettingsDialogResult initialRecordSettingsDialogResult(
    const RecordSettingsDialogInput& input
)
{
    return sanitizedRecordSettingsDialogResult(
        input.initialDirectory,
        input.initialRecordDelaySeconds,
        input.initialExportSampleRate,
        input.initialSaveFormat,
        input.defaultExportSampleRate
    );
}

} // namespace ovtr::win32
