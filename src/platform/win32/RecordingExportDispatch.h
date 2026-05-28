#pragma once

#include "data/SessionTypes.h"
#include "export/ExportTypes.h"
#include "platform/win32/ConfigTypes.h"

#include <filesystem>

namespace ovtr::win32 {

ovtr::ExportResult exportRecordingSession(
    const ovtr::RecordingSession& session,
    ExportFormat format,
    const std::filesystem::path& exportDirectory,
    double exportSampleRate
);

} // namespace ovtr::win32
