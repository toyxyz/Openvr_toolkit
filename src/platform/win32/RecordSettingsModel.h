#pragma once

#include "platform/win32/ConfigTypes.h"

#include <filesystem>

namespace ovtr::win32 {

struct RecordSettingsDialogInput {
    std::filesystem::path initialDirectory;
    float initialRecordDelaySeconds = 0.0f;
    float initialExportSampleRate = 60.0f;
    ExportFormat initialSaveFormat = ExportFormat::Glb;
    float defaultExportSampleRate = 60.0f;
};

struct RecordSettingsDialogResult {
    std::filesystem::path directory;
    float recordDelaySeconds = 0.0f;
    float exportSampleRate = 60.0f;
    ExportFormat saveFormat = ExportFormat::Glb;
};

RecordSettingsDialogResult initialRecordSettingsDialogResult(
    const RecordSettingsDialogInput& input
);

RecordSettingsDialogResult sanitizedRecordSettingsDialogResult(
    const std::filesystem::path& directory,
    float recordDelaySeconds,
    float exportSampleRate,
    ExportFormat saveFormat,
    float defaultExportSampleRate
);

} // namespace ovtr::win32
