#pragma once

#include "data/SessionTypes.h"

#include <filesystem>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppRecordingState;
struct AppRuntimeState;

struct RecordingExportPlan {
    ovtr::RecordingSession session;
    std::filesystem::path exportDirectory;
    double exportSampleRate = 60.0;
};

RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
);

} // namespace ovtr::win32
