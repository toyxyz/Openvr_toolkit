#pragma once

#include "export/ExportPoseTrack.h"
#include "export/FbxAsciiExporter.h"
#include "export/FbxExportModel.h"

#include <vector>

namespace ovtr::detail {

FbxPoseKey makeFbxPoseKey(const ExportPoseKey& sourceKey, FbxCoordinatePolicy coordinatePolicy);
void resampleFbxPoseKeys(std::vector<FbxPoseKey>& keys, double sampleRate);
void applyFbxEulerDiscontinuityFilter(std::vector<FbxPoseKey>& keys);

} // namespace ovtr::detail
