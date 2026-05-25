#pragma once

#include "data/SessionTypes.h"
#include "export/ExportTypes.h"
#include "export/RenderModelGeometry.h"

#include <filesystem>
#include <functional>

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
    std::function<RenderModelGeometry(const DeviceDescriptor&)> geometryProvider;
};

ExportResult exportSessionToGltf(
    const RecordingSession& session,
    const GltfExportOptions& options
);

} // namespace ovtr
