#pragma once

#include "export/FbxAsciiExporter.h"
#include "export/FbxExportModel.h"
#include "export/FbxTimeline.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr {

void writeFbxHeader(std::ostream& out, FbxCoordinatePolicy policy, const FbxTimelineSettings& timeline);
void writeFbxModel(std::ostream& out, const FbxDeviceExport& device);
void writeFbxRootModel(std::ostream& out, std::int64_t rootModelId);
void writeFbxAnimationStack(std::ostream& out, std::int64_t stackId, const FbxTimelineSettings& timeline);
void writeFbxGeometry(std::ostream& out, const FbxDeviceExport& device);
void writeFbxCurve(
    std::ostream& out,
    std::int64_t id,
    const std::string& name,
    const std::vector<FbxPoseKey>& keys,
    bool rotation,
    int axis
);
void writeFbxCurveNode(std::ostream& out, std::int64_t id, const std::string& name, bool rotation);
void writeFbxConnections(
    std::ostream& out,
    const std::vector<FbxDeviceExport>& devices,
    std::int64_t rootModelId,
    std::int64_t stackId,
    std::int64_t layerId
);

} // namespace ovtr
