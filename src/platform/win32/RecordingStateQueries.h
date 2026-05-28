#pragma once

#include "recording/RecordingState.h"

#include <filesystem>
#include <string>

namespace ovtr::win32 {

struct AppRecordingState;

bool isRecorderBusyForExport(bool recordingDelayActive, RecorderState recorderState) noexcept;
bool isRecorderBusyForExport(const AppRecordingState& state);
std::string exportBlockReason(
    bool recordingDelayActive,
    RecorderState recorderState,
    const std::filesystem::path& currentSessionFolder
);
std::string exportBlockReason(const AppRecordingState& state);
bool isRecordingControlActive(bool recordingDelayActive, RecorderState recorderState) noexcept;
bool isRecordingControlActive(const AppRecordingState& state);

} // namespace ovtr::win32
