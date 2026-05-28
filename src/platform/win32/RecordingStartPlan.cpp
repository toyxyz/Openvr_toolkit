#include "platform/win32/RecordingStartPlan.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/RecordingSessionActions.h"

namespace ovtr::win32 {

RecordingStartPlan makeRecordingStartPlan(
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState,
    const std::filesystem::path& recordingsRoot,
    const std::string& sessionId,
    const std::string& createdAtUtc,
    const double targetSampleRate
)
{
    ovtr::RecordingSession session = makeRecordingSession(
        sessionId,
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

} // namespace ovtr::win32
