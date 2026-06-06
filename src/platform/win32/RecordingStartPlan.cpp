#include "platform/win32/RecordingStartPlan.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/RecordingSessionActions.h"
#include "platform/win32/Win32String.h"

#include <system_error>

namespace ovtr::win32 {
namespace {

std::string uniqueRecordingSessionId(
    const std::filesystem::path& recordingsRoot,
    const std::string& baseSessionId
)
{
    std::error_code error;
    if (!std::filesystem::exists(recordingSessionFolder(recordingsRoot, baseSessionId), error) && !error) {
        return baseSessionId;
    }
    for (int suffix = 0; suffix < 100000; ++suffix) {
        const std::string candidate = baseSessionId + "_" + std::to_string(suffix);
        error.clear();
        if (!std::filesystem::exists(recordingSessionFolder(recordingsRoot, candidate), error) && !error) {
            return candidate;
        }
    }
    return baseSessionId + "_99999";
}

} // namespace

RecordingStartPlan makeRecordingStartPlan(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const std::filesystem::path& recordingsRoot,
    const std::string& sessionId,
    const std::string& createdAtUtc,
    const double targetSampleRate
)
{
    const std::string uniqueSessionId = uniqueRecordingSessionId(recordingsRoot, sessionId);
    ovtr::RecordingSession session = makeRecordingSession(
        uniqueSessionId,
        createdAtUtc,
        targetSampleRate,
        runtimeState.devices
    );
    applyCustomNamesToExportDevices(deviceState, session.devices);

    RecordingStartPlan plan;
    plan.sessionFolder = recordingSessionFolder(recordingsRoot, session.sessionId);
    plan.options.sessionFolder = plan.sessionFolder;
    plan.options.session = std::move(session);
    return plan;
}

std::string recordingSessionIdForName(
    const std::wstring& sessionName,
    const std::string& timestamp
)
{
    const std::wstring sanitizedName = sanitizedSessionFolderName(sessionName);
    if (sanitizedName.empty()) {
        return "session_" + timestamp;
    }
    return narrow(sanitizedName);
}

} // namespace ovtr::win32
