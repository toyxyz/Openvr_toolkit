#pragma once

#include "export/FbxExportModel.h"
#include "export/FbxTimeline.h"

#include <vector>

namespace ovtr::detail {

std::vector<FbxTimelineKeyRange> collectFbxTimelineKeyRanges(const std::vector<FbxDeviceExport>& devices);

} // namespace ovtr::detail
