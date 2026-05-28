#pragma once

#include "export/ExportPoseTrack.h"
#include "export/FbxAsciiExporter.h"
#include "export/FbxExportModel.h"
#include "export/FbxTimeline.h"

#include <cstdint>
#include <vector>

namespace ovtr {

struct FbxExportScene {
    std::vector<FbxDeviceExport> devices;
    FbxTimelineSettings timeline;
    std::int64_t nextId = 1'000'000;
};

void buildFbxExportScene(
    std::vector<ExportPoseTrack> tracks,
    FbxCoordinatePolicy coordinatePolicy,
    double exportSampleRate,
    double targetSampleRate,
    FbxExportScene& scene
);

} // namespace ovtr
