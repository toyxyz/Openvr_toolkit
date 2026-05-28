#pragma once

#include "data/SessionTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ovtr::win32 {

RecordingSession makeRecordingSession(
    const std::string& sessionId,
    const std::string& createdAtUtc,
    double targetSampleRate,
    std::vector<DeviceDescriptor> devices
);
std::filesystem::path recordingSessionFolder(
    const std::filesystem::path& recordingsRoot,
    const std::string& sessionId
);
RecordingSession prepareSessionForExport(
    RecordingSession session,
    const std::filesystem::path& currentSessionFolder,
    const std::vector<DeviceDescriptor>& fallbackDevices
);

} // namespace ovtr::win32
