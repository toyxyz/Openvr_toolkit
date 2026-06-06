#pragma once

#include "recording/RecordingController.h"

#include <filesystem>
#include <string>

namespace ovtr::win32 {

struct AppDeviceState;
struct AppRuntimeState;

struct RecordingStartPlan {
    std::filesystem::path sessionFolder;
    ovtr::RecordingStartOptions options;
};

RecordingStartPlan makeRecordingStartPlan(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const std::filesystem::path& recordingsRoot,
    const std::string& sessionId,
    const std::string& createdAtUtc,
    double targetSampleRate
);
std::string recordingSessionIdForName(
    const std::wstring& sessionName,
    const std::string& timestamp
);

} // namespace ovtr::win32
