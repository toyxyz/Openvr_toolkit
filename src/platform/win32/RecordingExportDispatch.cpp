#include "platform/win32/RecordingExportDispatch.h"

#include "export/FbxAsciiExporter.h"
#include "export/GltfExporter.h"

namespace ovtr::win32 {

ovtr::ExportResult exportRecordingSession(
    const ovtr::RecordingSession& session,
    const ExportFormat format,
    const std::filesystem::path& exportDirectory,
    const double exportSampleRate
)
{
    if (format == ExportFormat::Fbx) {
        ovtr::FbxExportOptions options;
        options.outputPath = exportDirectory / (session.sessionId + ".fbx");
        options.includeGeometry = true;
        options.includeTrackingReference = true;
        options.exportSampleRate = exportSampleRate;
        return ovtr::exportSessionToFbxAscii(session, options);
    }

    ovtr::GltfExportOptions options;
    options.outputPath = exportDirectory / (session.sessionId + ".glb");
    options.includeTrackingReference = true;
    options.exportSampleRate = exportSampleRate;
    options.format = ovtr::GltfExportFormat::Glb;
    return ovtr::exportSessionToGltf(session, options);
}

} // namespace ovtr::win32
