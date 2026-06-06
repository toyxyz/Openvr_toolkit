#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct RecordingExportUiMessages {
    std::string statusMessage;
    std::vector<std::string> logMessages;
};

std::wstring recordingExportStartLogMessage();
std::string recordingExportSuccessMessage(const std::filesystem::path& outputPath);
std::string recordingExportFailureMessage(const std::string& error);
RecordingExportUiMessages recordingExportSuccessUiMessages(
    const std::filesystem::path& outputPath,
    bool cleanupSucceeded,
    const std::string& cleanupMessage
);
RecordingExportUiMessages recordingExportFailureUiMessages(const std::string& error);

} // namespace ovtr::win32
