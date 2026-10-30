#pragma once

#include "data/SessionTypes.h"
#include "export/ExportTypes.h"

#include <filesystem>

namespace ovtr::win32 {

ovtr::ExportResult exportSessionVideo(
    const ovtr::RecordingSession& session,
    const std::filesystem::path& exportDirectory
);

} // namespace ovtr::win32
