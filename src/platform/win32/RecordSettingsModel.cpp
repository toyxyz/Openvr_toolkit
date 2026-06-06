#include "platform/win32/RecordSettingsModel.h"

#include "platform/win32/ConfigStore.h"

namespace ovtr::win32 {

RecordSettingsDialogResult sanitizedRecordSettingsDialogResult(
    const std::filesystem::path& directory,
    const float recordDelaySeconds,
    const float exportSampleRate,
    const bool startRecordingOnCalibration,
    const bool exportAfterRecording,
    const bool applyNoiseFilterOnExport,
    const float noiseFilterCutoffHz,
    const OutlierRepairStrength outlierRepairStrength,
    const int smoothingIterations,
    const float defaultExportSampleRate
)
{
    RecordSettingsDialogResult result;
    result.directory = normalizedExportDirectoryPath(directory);
    result.recordDelaySeconds = sanitizedRecordDelaySeconds(recordDelaySeconds);
    result.exportSampleRate = sanitizedExportSampleRate(exportSampleRate, defaultExportSampleRate);
    result.startRecordingOnCalibration = startRecordingOnCalibration;
    result.exportAfterRecording = exportAfterRecording;
    result.applyNoiseFilterOnExport = applyNoiseFilterOnExport;
    result.noiseFilterCutoffHz = sanitizedNoiseFilterCutoffHz(noiseFilterCutoffHz);
    result.outlierRepairStrength = outlierRepairStrength;
    result.smoothingIterations = sanitizedSmoothingIterations(smoothingIterations);
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
        input.initialStartRecordingOnCalibration,
        input.initialExportAfterRecording,
        input.initialApplyNoiseFilterOnExport,
        input.initialNoiseFilterCutoffHz,
        input.initialOutlierRepairStrength,
        input.initialSmoothingIterations,
        input.defaultExportSampleRate
    );
}

} // namespace ovtr::win32
