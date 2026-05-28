#pragma once

#include "export/ExportPoseTrack.h"
#include "platform/win32/AppMarkerState.h"

#include <vector>

namespace ovtr::win32 {

std::vector<ovtr::ExportStaticPoseTrack> makeMarkerStaticExportTracks(const AppMarkerState& markerState);

} // namespace ovtr::win32
