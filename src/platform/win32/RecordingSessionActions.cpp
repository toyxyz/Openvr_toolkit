#include "platform/win32/RecordingSessionActions.h"

#include <utility>

namespace ovtr::win32 {

RecordingSession makeRecordingSession(
    const std::string& sessionId,
    const std::string& createdAtUtc,
    const double targetSampleRate,
    std::vector<DeviceDescriptor> devices
)
{
    RecordingSession session;
    session.sessionId = sessionId;
    session.sessionName = sessionId;
    session.createdAtUtc = createdAtUtc;
    session.appVersion = "0.1.0";
    session.targetSampleRate = targetSampleRate;
    session.devices = std::move(devices);
    return session;
}

std::filesystem::path recordingSessionFolder(
    const std::filesystem::path& recordingsRoot,
    const std::string& sessionId
)
{
    return recordingsRoot / sessionId;
}

RecordingSession prepareSessionForExport(
    RecordingSession session,
    const std::filesystem::path& currentSessionFolder,
    const std::vector<DeviceDescriptor>& fallbackDevices
)
{
    if (session.sessionId.empty()) {
        session.sessionId = currentSessionFolder.filename().string();
        session.sessionName = session.sessionId;
    }
    if (session.devices.empty()) {
        session.devices = fallbackDevices;
    }
    if (session.framesPath.empty()) {
        session.framesPath = currentSessionFolder / "frames.bin";
    }
    if (session.frameIndexPath.empty()) {
        session.frameIndexPath = currentSessionFolder / "frame_index.bin";
    }
    return session;
}

} // namespace ovtr::win32
