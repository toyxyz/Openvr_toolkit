#pragma once

#include "platform/win32/ConfigTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct RecordingExportUiMessages {
    std::string statusMessage;
    std::vector<std::string> logMessages;
};

std::wstring recordingExportStartLogMessage(ExportFormat format);
std::string recordingExportSuccessMessage(ExportFormat format, const std::filesystem::path& outputPath);
std::string recordingExportFailureMessage(ExportFormat format, const std::string& error);
RecordingExportUiMessages recordingExportSuccessUiMessages(
    ExportFormat format,
    const std::filesystem::path& outputPath,
    bool cleanupSucceeded,
    const std::string& cleanupMessage
);
RecordingExportUiMessages recordingExportFailureUiMessages(ExportFormat format, const std::string& error);

} // namespace ovtr::win32
