#include "platform/win32/RecordingExportPlan.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/RecordingSessionActions.h"

#include <utility>

namespace ovtr::win32 {

RecordingExportPlan makeRecordingExportPlan(
    const AppRecordingState& recordingState,
    const AppRuntimeState& runtimeState,
    const AppDeviceState& deviceState
)
{
    RecordingExportPlan plan;
    plan.session = prepareSessionForExport(
        recordingState.recorder.session(),
        recordingState.currentSessionFolder,
        runtimeState.devices
    );
    applyCustomNamesToExportDevices(deviceState, plan.session.devices);
    plan.exportDirectory = normalizedExportDirectoryPath(recordingState.exportDirectory);
    plan.exportSampleRate = static_cast<double>(
        sanitizedExportSampleRate(recordingState.recordExportSampleRate, kDefaultRecordExportSampleRate)
    );
    return plan;
}

} // namespace ovtr::win32
