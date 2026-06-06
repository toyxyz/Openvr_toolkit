#pragma once

#include "data/SessionTypes.h"
#include "export/ExportPoseTrack.h"
#include "export/ExportTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

std::filesystem::path uniqueExportOutputPath(
    const std::filesystem::path& exportDirectory,
    const std::string& baseName,
    const std::string& extension
);

ovtr::ExportResult exportRecordingSession(
    const ovtr::RecordingSession& session,
    const std::filesystem::path& exportDirectory,
    double exportSampleRate,
    const std::vector<ovtr::ExportStaticPoseTrack>& staticTracks = {}
);

} // namespace ovtr::win32
