#include "platform/win32/RecordingExportMessages.h"

namespace ovtr::win32 {

std::wstring recordingExportStartLogMessage(const ExportFormat format)
{
    return format == ExportFormat::Fbx ? L"Starting FBX export" : L"Starting GLB export";
}

std::string recordingExportSuccessMessage(
    const ExportFormat format,
    const std::filesystem::path& outputPath
)
{
    return (format == ExportFormat::Fbx ? "FBX saved to " : "GLB saved to ") + outputPath.string();
}

std::string recordingExportFailureMessage(const ExportFormat format, const std::string& error)
{
    return (format == ExportFormat::Fbx ? "FBX export failed: " : "GLB export failed: ") + error;
}

RecordingExportUiMessages recordingExportSuccessUiMessages(
    const ExportFormat format,
    const std::filesystem::path& outputPath,
    const bool cleanupSucceeded,
    const std::string& cleanupMessage
)
{
    RecordingExportUiMessages messages;
    messages.statusMessage = recordingExportSuccessMessage(format, outputPath);
    messages.logMessages.push_back(messages.statusMessage);
    if (!cleanupMessage.empty()) {
        messages.logMessages.push_back(cleanupMessage);
        if (!cleanupSucceeded) {
            messages.statusMessage += " (" + cleanupMessage + ")";
        }
    }
    return messages;
}

RecordingExportUiMessages recordingExportFailureUiMessages(
    const ExportFormat format,
    const std::string& error
)
{
    RecordingExportUiMessages messages;
    messages.statusMessage = recordingExportFailureMessage(format, error);
    messages.logMessages.push_back(messages.statusMessage);
    return messages;
}

} // namespace ovtr::win32
