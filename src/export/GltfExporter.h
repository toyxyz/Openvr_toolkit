#pragma once

#include "data/SessionTypes.h"
#include "export/ExportPoseTrack.h"
#include "export/ExportTypes.h"

#include <filesystem>

namespace ovtr {

enum class GltfExportFormat {
    Gltf,
    Glb,
};

struct GltfExportOptions {
    std::filesystem::path outputPath;
    bool includeGeometry = true;
    bool includeTrackingReference = true;
    double exportSampleRate = 0.0;
    GltfExportFormat format = GltfExportFormat::Glb;
    ExportGeometryProvider geometryProvider;
};

ExportResult exportSessionToGltf(
    const RecordingSession& session,
    const GltfExportOptions& options
);

} // namespace ovtr
