#pragma once

#include "data/SessionTypes.h"
#include "export/ExportPoseTrack.h"
#include "export/ExportTypes.h"
#include "platform/win32/ConfigTypes.h"

#include <filesystem>
#include <vector>

namespace ovtr::win32 {

ovtr::ExportResult exportRecordingSession(
    const ovtr::RecordingSession& session,
    ExportFormat format,
    const std::filesystem::path& exportDirectory,
    double exportSampleRate,
    const std::vector<ovtr::ExportStaticPoseTrack>& staticTracks = {}
);

} // namespace ovtr::win32
