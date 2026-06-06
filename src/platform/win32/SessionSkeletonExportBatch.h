#pragma once

#include "platform/win32/ExportProgressWorker.h"
#include "platform/win32/RecordingExportMessages.h"
#include "platform/win32/SessionSkeletonExportClip.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

void exportSkeletonGlbsForActors(
    const std::vector<SessionSkeletonClipRequest>& requests,
    const std::filesystem::path& exportDirectory,
    const std::string& sessionStem,
    const ExportProgressReporter& reporter,
    RecordingExportUiMessages& messages
);

} // namespace ovtr::win32
