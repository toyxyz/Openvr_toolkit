#pragma once

#include "export/ExportPoseTrack.h"

#include <string>
#include <vector>

namespace ovtr {

bool applySkeletalExportHierarchy(std::vector<ExportPoseTrack>& tracks, std::string& error);

} // namespace ovtr
