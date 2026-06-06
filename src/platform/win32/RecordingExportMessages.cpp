#include "platform/win32/RecordingExportMessages.h"

namespace ovtr::win32 {

std::wstring recordingExportStartLogMessage()
{
    return L"Starting GLB export";
}

std::string recordingExportSuccessMessage(const std::filesystem::path& outputPath)
{
    return "GLB saved to " + outputPath.string();
}

std::string recordingExportFailureMessage(const std::string& error)
{
    return "GLB export failed: " + error;
}

RecordingExportUiMessages recordingExportSuccessUiMessages(
    const std::filesystem::path& outputPath,
    const bool cleanupSucceeded,
    const std::string& cleanupMessage
)
{
    RecordingExportUiMessages messages;
    messages.statusMessage = recordingExportSuccessMessage(outputPath);
    messages.logMessages.push_back(messages.statusMessage);
    if (!cleanupMessage.empty()) {
        messages.logMessages.push_back(cleanupMessage);
        if (!cleanupSucceeded) {
            messages.statusMessage += " (" + cleanupMessage + ")";
        }
    }
    return messages;
}

RecordingExportUiMessages recordingExportFailureUiMessages(const std::string& error)
{
    RecordingExportUiMessages messages;
    messages.statusMessage = recordingExportFailureMessage(error);
    messages.logMessages.push_back(messages.statusMessage);
    return messages;
}

} // namespace ovtr::win32
