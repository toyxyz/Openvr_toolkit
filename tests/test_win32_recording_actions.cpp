#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppDeviceState.h"
#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/RecordingExportMessages.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/RecordingSessionActions.h"
#include "platform/win32/RecordingStartPlan.h"
#include "platform/win32/RecordingStateQueries.h"

#include <filesystem>

namespace ovtr::test {

void testWin32RecordingActions()
{
    require(
        ovtr::win32::isRecorderBusyForExport(true, ovtr::RecorderState::Idle),
        "record delay blocks export"
    );
    require(
        ovtr::win32::isRecorderBusyForExport(false, ovtr::RecorderState::Recording),
        "active recording blocks export"
    );
    require(
        !ovtr::win32::isRecorderBusyForExport(false, ovtr::RecorderState::Idle),
        "idle recorder permits export"
    );

    require(
        ovtr::win32::exportBlockReason(true, ovtr::RecorderState::Idle, "session_1") ==
            "stop recording before exporting",
        "export block reason for delay"
    );
    require(
        ovtr::win32::exportBlockReason(false, ovtr::RecorderState::Idle, {}) ==
            "no recorded session available",
        "export block reason for missing session"
    );
    require(
        ovtr::win32::exportBlockReason(false, ovtr::RecorderState::Idle, "session_1").empty(),
        "export should be available for idle recorder with session"
    );

    require(
        ovtr::win32::isRecordingControlActive(true, ovtr::RecorderState::Idle),
        "delayed recording activates control"
    );
    require(
        ovtr::win32::isRecordingControlActive(false, ovtr::RecorderState::Paused),
        "paused recording activates control"
    );
    require(
        !ovtr::win32::isRecordingControlActive(false, ovtr::RecorderState::Finalizing),
        "finalizing is not a pressed recording control state"
    );

    ovtr::win32::AppRecordingState recordingState;
    recordingState.recordingDelayActive = true;
    recordingState.currentSessionFolder = "session_state";
    require(
        ovtr::win32::isRecorderBusyForExport(recordingState),
        "recording state delay blocks export"
    );
    require(
        ovtr::win32::exportBlockReason(recordingState) == "stop recording before exporting",
        "recording state export block reason"
    );
    require(
        ovtr::win32::isRecordingControlActive(recordingState),
        "recording state delay activates control"
    );

    ovtr::DeviceDescriptor tracker = makeTestTracker("LHR-ACTION");
    tracker.id = 7;

    const ovtr::RecordingSession newSession = ovtr::win32::makeRecordingSession(
        "session_action",
        "2026-05-27T12:00:00Z",
        90.0,
        {tracker}
    );
    require(newSession.sessionId == "session_action", "recording session id");
    require(newSession.sessionName == "session_action", "recording session name");
    require(newSession.createdAtUtc == "2026-05-27T12:00:00Z", "recording session timestamp");
    require(newSession.targetSampleRate == 90.0, "recording session target sample rate");
    require(newSession.devices.size() == 1, "recording session device count");

    const std::filesystem::path root = std::filesystem::current_path() / ".tmp_ovtr_recording_actions";
    const std::filesystem::path folder = ovtr::win32::recordingSessionFolder(root, "session_action");
    require(folder == root / "session_action", "recording session folder");

    ovtr::RecordingSession exportSession;
    exportSession = ovtr::win32::prepareSessionForExport(exportSession, folder, {tracker});
    require(exportSession.sessionId == "session_action", "export session fills id from folder");
    require(exportSession.sessionName == "session_action", "export session fills name from folder");
    require(exportSession.devices.size() == 1, "export session fills devices");
    require(exportSession.framesPath == folder / "frames.bin", "export session fills frames path");
    require(exportSession.frameIndexPath == folder / "frame_index.bin", "export session fills index path");

    ovtr::win32::AppRuntimeState runtimeState;
    ovtr::win32::AppDeviceState deviceState;
    tracker.serial = "LHR-PLAN";
    tracker.deviceClass = ovtr::DeviceClass::GenericTracker;
    runtimeState.devices = {tracker};
    deviceState.deviceCustomNames[ovtr::win32::deviceNameKeyForDevice(tracker)] = "Named tracker";
    const ovtr::win32::RecordingStartPlan plan = ovtr::win32::makeRecordingStartPlan(
        runtimeState,
        deviceState,
        root,
        "session_plan",
        "2026-05-27T13:00:00Z",
        120.0
    );
    require(plan.sessionFolder == root / "session_plan", "recording start plan folder");
    require(plan.options.sessionFolder == plan.sessionFolder, "recording start options folder");
    require(plan.options.session.sessionId == "session_plan", "recording start plan session id");
    require(plan.options.session.targetSampleRate == 120.0, "recording start plan sample rate");
    require(plan.options.session.devices.size() == 1, "recording start plan device count");
    require(plan.options.session.devices[0].displayName == "Named tracker", "recording start plan custom names");

    recordingState.currentSessionFolder = root / "session_export_plan";
    recordingState.exportDirectory = root;
    recordingState.recordExportSampleRate = 180.0f;
    ovtr::win32::RecordingExportPlan exportPlan =
        ovtr::win32::makeRecordingExportPlan(recordingState, runtimeState, deviceState);
    require(exportPlan.session.sessionId == "session_export_plan", "recording export plan fills session id");
    require(exportPlan.session.devices.size() == 1, "recording export plan device count");
    require(exportPlan.session.devices[0].displayName == "Named tracker", "recording export plan custom names");
    require(exportPlan.exportDirectory == root, "recording export plan uses session parent directory fallback");
    require(exportPlan.exportSampleRate == 180.0, "recording export plan sample rate");
    require(
        ovtr::win32::sanitizedSessionFolderName(L"  Project:01  ") == L"Project_01",
        "session folder name trims and sanitizes invalid characters"
    );
    require(
        ovtr::win32::sanitizedSessionFolderName(L"CON") == L"_CON",
        "session folder name avoids reserved device names"
    );
    require(
        ovtr::win32::sessionExportDirectoryPath(root, L"  ") == root.lexically_normal(),
        "blank session export path uses base export folder"
    );
    require(
        ovtr::win32::sessionExportDirectoryPath(root, L"Take/01") ==
            (root / std::filesystem::path(L"Take_01")).lexically_normal(),
        "session export path uses sanitized session folder"
    );
    ovtr::win32::RecordingExportPlan sessionExportPlan =
        ovtr::win32::makeRecordingExportPlan(recordingState, runtimeState, deviceState, L" Session: A ");
    require(
        sessionExportPlan.exportDirectory == (root / std::filesystem::path(L"Session_ A")).lexically_normal(),
        "recording export plan uses session folder when provided"
    );

    const ovtr::win32::RecordingExportUiMessages successMessages =
        ovtr::win32::recordingExportSuccessUiMessages(
            ovtr::win32::ExportFormat::Glb,
            root / "session.glb",
            true,
            "Temporary session folder deleted: session"
        );
    require(
        successMessages.statusMessage == "GLB saved to " + (root / "session.glb").string(),
        "recording export success status ignores successful cleanup details"
    );
    require(successMessages.logMessages.size() == 2, "recording export success logs cleanup");

    const ovtr::win32::RecordingExportUiMessages cleanupFailedMessages =
        ovtr::win32::recordingExportSuccessUiMessages(
            ovtr::win32::ExportFormat::Fbx,
            root / "session.fbx",
            false,
            "session cleanup skipped: folder is outside recordings"
        );
    require(
        cleanupFailedMessages.statusMessage.find("session cleanup skipped: folder is outside recordings") !=
            std::string::npos,
        "recording export status includes failed cleanup"
    );

    const ovtr::win32::RecordingExportUiMessages failureMessages =
        ovtr::win32::recordingExportFailureUiMessages(ovtr::win32::ExportFormat::Glb, "bad session");
    require(failureMessages.statusMessage == "GLB export failed: bad session", "recording export failure status");
    require(failureMessages.logMessages.size() == 1, "recording export failure logs once");
}

} // namespace ovtr::test
