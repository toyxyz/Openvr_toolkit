#include "platform/win32/RecordingExportDispatch.h"

#include "export/GltfExporter.h"

#include <cstdint>
#include <string>
#include <system_error>

namespace ovtr::win32 {

std::filesystem::path uniqueExportOutputPath(
    const std::filesystem::path& exportDirectory,
    const std::string& baseName,
    const std::string& extension
)
{
    const std::filesystem::path firstPath = exportDirectory / (baseName + extension);
    std::error_code error;
    if (!std::filesystem::exists(firstPath, error)) {
        return firstPath;
    }
    for (std::uint64_t suffix = 0;; ++suffix) {
        const std::filesystem::path candidate =
            exportDirectory / (baseName + "_" + std::to_string(suffix) + extension);
        error.clear();
        if (!std::filesystem::exists(candidate, error)) {
            return candidate;
        }
    }
}

ovtr::ExportResult exportRecordingSession(
    const ovtr::RecordingSession& session,
    const std::filesystem::path& exportDirectory,
    const double exportSampleRate,
    const std::vector<ovtr::ExportStaticPoseTrack>& staticTracks
)
{
    ovtr::GltfExportOptions options;
    options.outputPath = uniqueExportOutputPath(exportDirectory, session.sessionId, ".glb");
    options.includeTrackingReference = true;
    options.exportSampleRate = exportSampleRate;
    options.format = ovtr::GltfExportFormat::Glb;
    options.staticTracks = staticTracks;
    return ovtr::exportSessionToGltf(session, options);
}

} // namespace ovtr::win32
