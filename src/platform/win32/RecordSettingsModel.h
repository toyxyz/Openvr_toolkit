#pragma once

#include "platform/win32/ConfigTypes.h"

#include <filesystem>

namespace ovtr::win32 {

struct RecordSettingsDialogInput {
    std::filesystem::path initialDirectory;
    std::filesystem::path initialSessionDirectory;
    float initialRecordDelaySeconds = 0.0f;
    float initialExportSampleRate = 60.0f;
    bool initialStartRecordingOnCalibration = false;
    bool initialExportAfterRecording = false;
    bool initialApplyNoiseFilterOnExport = false;
    float initialNoiseFilterCutoffHz = 8.0f;
    OutlierRepairStrength initialOutlierRepairStrength = OutlierRepairStrength::Light;
    int initialSmoothingIterations = 0;
    float defaultExportSampleRate = 60.0f;
};

struct RecordSettingsDialogResult {
    std::filesystem::path directory;
    std::filesystem::path sessionDirectory;
    float recordDelaySeconds = 0.0f;
    float exportSampleRate = 60.0f;
    bool startRecordingOnCalibration = false;
    bool exportAfterRecording = false;
    bool applyNoiseFilterOnExport = false;
    float noiseFilterCutoffHz = 8.0f;
    OutlierRepairStrength outlierRepairStrength = OutlierRepairStrength::Light;
    int smoothingIterations = 0;
};

RecordSettingsDialogResult initialRecordSettingsDialogResult(
    const RecordSettingsDialogInput& input
);

RecordSettingsDialogResult sanitizedRecordSettingsDialogResult(
    const std::filesystem::path& directory,
    const std::filesystem::path& sessionDirectory,
    float recordDelaySeconds,
    float exportSampleRate,
    bool startRecordingOnCalibration,
    bool exportAfterRecording,
    bool applyNoiseFilterOnExport,
    float noiseFilterCutoffHz,
    OutlierRepairStrength outlierRepairStrength,
    int smoothingIterations,
    float defaultExportSampleRate
);

} // namespace ovtr::win32
