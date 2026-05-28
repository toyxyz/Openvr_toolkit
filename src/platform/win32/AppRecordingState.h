#pragma once

#include "platform/win32/ConfigTypes.h"
#include "recording/RecordingController.h"
#include "recording/SamplingScheduler.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>

namespace ovtr::win32 {

struct AppRecordingState {
    mutable std::mutex recordingMutex;
    RecordingController recorder;
    SamplingScheduler recordingScheduler{90.0};
    std::chrono::steady_clock::time_point recordingStart = std::chrono::steady_clock::now();
    std::uint64_t recordingDroppedFrames = 0;
    std::filesystem::path currentSessionFolder;
    std::string recordingError;
    std::string exportStatusMessage;
    std::filesystem::path exportDirectory;
    float recordDelaySeconds = 0.0f;
    float recordExportSampleRate = 60.0f;
    ExportFormat recordSaveFormat = ExportFormat::Glb;
    bool recordingDelayActive = false;
    std::chrono::steady_clock::time_point recordingDelayDeadline{};
};

} // namespace ovtr::win32
