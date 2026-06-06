#pragma once

#include "platform/win32/ConfigTypes.h"
#include "platform/win32/SkeletonRecordingTypes.h"
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
    std::filesystem::path sessionDirectory;
    float recordDelaySeconds = 0.0f;
    float recordExportSampleRate = 60.0f;
    bool startRecordingOnCalibration = false;
    bool exportAfterRecording = false;
    bool applyNoiseFilterOnExport = false;
    float noiseFilterCutoffHz = 8.0f;
    OutlierRepairStrength outlierRepairStrength = OutlierRepairStrength::Light;
    int smoothingIterations = 0;
    SkeletonRecordingClip skeletonRecording;
    bool recordingDelayActive = false;
    std::chrono::steady_clock::time_point recordingDelayDeadline{};
};

} // namespace ovtr::win32
