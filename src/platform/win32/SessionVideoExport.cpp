#include "platform/win32/SessionVideoExport.h"
#include "platform/win32/RecordingExportDispatch.h"

#include <filesystem>

namespace ovtr::win32 {

ovtr::ExportResult exportSessionVideo(
    const ovtr::RecordingSession& session,
    const std::filesystem::path& exportDirectory
)
{
    if (session.videoPath.empty()) {
        return {false, {}, "session does not contain video"};
    }
    std::error_code error;
    if (!std::filesystem::is_regular_file(session.videoPath, error)) {
        return {false, {}, error ? "failed to inspect session video: " + error.message() :
            "session video file is missing"};
    }
    std::filesystem::create_directories(exportDirectory, error);
    if (error) {
        return {false, {}, "failed to create video export directory: " + error.message()};
    }
    const std::filesystem::path output = uniqueExportOutputPath(
        exportDirectory, session.sessionId.empty() ? "session_video" : session.sessionId, ".mp4"
    );
    std::filesystem::copy_file(session.videoPath, output, std::filesystem::copy_options::none, error);
    if (error) {
        return {false, {}, "failed to export session video: " + error.message()};
    }
    return {true, output, {}};
}

} // namespace ovtr::win32
