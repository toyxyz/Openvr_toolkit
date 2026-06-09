#include "platform/win32/SessionExportActions.h"

#include "platform/win32/AppConfig.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ConfigStore.h"
#include "platform/win32/DeviceList.h"
#include "platform/win32/ExportProgressWorker.h"
#include "platform/win32/RecordingExportDispatch.h"
#include "platform/win32/RecordingExportMessages.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/SessionSkeletonExportBatch.h"
#include "platform/win32/SessionSkeletonExportClip.h"
#include "platform/win32/SkeletonGltfExporter.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/WindowLayout.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace ovtr::win32 {
namespace {

ExportProgressResult progressResult(RecordingExportUiMessages messages)
{
    ExportProgressResult result;
    result.statusMessage = std::move(messages.statusMessage);
    result.logMessages = std::move(messages.logMessages);
    return result;
}

ovtr::RecordingSession loadedExportSession(AppWindowState& state)
{
    ovtr::RecordingSession session = state.loadedSession;
    if (session.sessionId.empty()) {
        session.sessionId = state.loadedSessionFolder.filename().string();
    }
    if (session.sessionName.empty()) {
        session.sessionName = session.sessionId;
    }
    applyCustomNamesToExportDevices(state, session.devices);
    return session;
}

void warnNoLoadedSession(HWND hwnd, AppWindowState& state, const wchar_t* title)
{
    MessageBoxW(hwnd, L"Load a session before exporting it.", title, MB_OK | MB_ICONWARNING);
    state.exportStatusMessage = "load a session before exporting";
    appendDebugLog(state, "Export session blocked: no loaded session");
    invalidateStatusPanel(hwnd);
}

bool blockExportIfBusy(HWND hwnd, AppWindowState& state)
{
    if (!isExportProgressActive(state)) {
        return false;
    }
    state.exportStatusMessage = "export already in progress";
    appendDebugLog(state, "Export blocked: export already in progress");
    invalidateStatusPanel(hwnd);
    return true;
}

SessionSkeletonClipRequest loadedSkeletonClipRequest(
    const ovtr::RecordingSession& session,
    const MappingActor& actor
) {
    SessionSkeletonClipRequest request;
    request.session = session;
    request.actor = actor;
    return request;
}

ExportProgressResult loadedDeviceExportWork(
    const ovtr::RecordingSession& session,
    const std::filesystem::path& exportDirectory,
    const double exportSampleRate,
    const ExportProgressReporter& reporter
) {
    reporter.update(0.10f, "Writing device GLB");
    const ovtr::ExportResult result =
        exportRecordingSession(session, exportDirectory, exportSampleRate, {});
    if (!result.success) {
        return progressResult(recordingExportFailureUiMessages(result.error));
    }

    RecordingExportUiMessages messages =
        recordingExportSuccessUiMessages(result.outputPath, true, {});
    reporter.update(1.0f, "Export complete");
    return progressResult(std::move(messages));
}

ExportProgressResult loadedPoseExportWork(
    const std::vector<SessionSkeletonClipRequest>& skeletonRequests,
    const std::filesystem::path& exportDirectory,
    const std::string& sessionStem,
    const ExportProgressReporter& reporter
) {
    RecordingExportUiMessages messages;
    if (skeletonRequests.empty()) {
        messages.statusMessage = "Skeleton GLB export skipped: no calibrated Mapping actor";
        messages.logMessages.push_back(messages.statusMessage);
        reporter.update(1.0f, "Export skipped");
        return progressResult(std::move(messages));
    }
    exportSkeletonGlbsForActors(
        skeletonRequests,
        exportDirectory,
        sessionStem,
        reporter,
        messages
    );
    if (messages.statusMessage.empty()) {
        messages.statusMessage = "Skeleton GLB export complete";
    }
    reporter.update(1.0f, "Export complete");
    return progressResult(std::move(messages));
}

} // namespace

const MappingActor* sessionSkeletonExportActor(const AppWindowState& state) noexcept
{
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.calibrated && actor.id == state.selectedMappingActorId) {
            return &actor;
        }
    }
    for (const MappingActor& actor : state.mappingActors) {
        if (actor.calibrated) {
            return &actor;
        }
    }
    return nullptr;
}

const MappingActor* loadedSessionSkeletonExportActor(const AppWindowState& state) noexcept
{
    return sessionSkeletonExportActor(state);
}

std::vector<SessionSkeletonClipRequest> sessionSkeletonClipRequests(
    const AppWindowState& state,
    const ovtr::RecordingSession& session
) {
    std::vector<SessionSkeletonClipRequest> requests;
    for (const MappingActor& actor : state.mappingActors) {
        if (!actor.calibrated) {
            continue;
        }
        SessionSkeletonClipRequest request = loadedSkeletonClipRequest(session, actor);
        request.applyNoiseFilterOnExport = state.applyNoiseFilterOnExport;
        request.noiseFilterCutoffHz = state.noiseFilterCutoffHz;
        request.outlierRepairStrength = state.outlierRepairStrength;
        request.smoothingIterations = state.smoothingIterations;
        requests.push_back(std::move(request));
    }
    return requests;
}

std::filesystem::path loadedSessionExportDirectory(const AppWindowState& state)
{
    return sessionExportDirectoryPath(state.exportDirectory, state.sessionName);
}

bool buildLoadedSessionSkeletonClip(
    AppWindowState& state,
    SkeletonRecordingClip& clip,
    std::string& error
) {
    const MappingActor* source = loadedSessionSkeletonExportActor(state);
    if (!source) {
        error = "no calibrated Mapping actor";
        return false;
    }
    SessionSkeletonClipRequest request =
        loadedSkeletonClipRequest(loadedExportSession(state), *source);
    request.applyNoiseFilterOnExport = state.applyNoiseFilterOnExport;
    request.noiseFilterCutoffHz = state.noiseFilterCutoffHz;
    request.outlierRepairStrength = state.outlierRepairStrength;
    request.smoothingIterations = state.smoothingIterations;
    return buildSessionSkeletonClipFromRequest(request, clip, error);
}

void exportLoadedDevice(HWND hwnd, AppWindowState& state)
{
    if (!state.loadedSessionActive) {
        warnNoLoadedSession(hwnd, state, L"Export Device");
        return;
    }

    if (blockExportIfBusy(hwnd, state)) {
        return;
    }

    ovtr::RecordingSession session = loadedExportSession(state);
    std::filesystem::path exportDirectory = loadedSessionExportDirectory(state);
    const double exportSampleRate = static_cast<double>(
        sanitizedExportSampleRate(state.recordExportSampleRate, kDefaultRecordExportSampleRate)
    );
    appendDebugLog(state, "Export device started: GLB");
    beginExportProgress(
        hwnd,
        state,
        "Exporting Device",
        [session = std::move(session),
         exportDirectory = std::move(exportDirectory),
         exportSampleRate](const ExportProgressReporter& reporter) {
            return loadedDeviceExportWork(
                session,
                exportDirectory,
                exportSampleRate,
                reporter
            );
        }
    );
}

void exportLoadedPose(HWND hwnd, AppWindowState& state)
{
    if (!state.loadedSessionActive) {
        warnNoLoadedSession(hwnd, state, L"Export Pose");
        return;
    }

    if (blockExportIfBusy(hwnd, state)) {
        return;
    }

    ovtr::RecordingSession session = loadedExportSession(state);
    std::vector<SessionSkeletonClipRequest> skeletonRequests =
        sessionSkeletonClipRequests(state, session);
    std::filesystem::path exportDirectory = loadedSessionExportDirectory(state);
    std::string sessionStem = session.sessionId;
    appendDebugLog(state, "Export pose started: skeleton GLB");
    beginExportProgress(
        hwnd,
        state,
        "Exporting Pose",
        [skeletonRequests = std::move(skeletonRequests),
         exportDirectory = std::move(exportDirectory),
         sessionStem = std::move(sessionStem)](const ExportProgressReporter& reporter) {
            return loadedPoseExportWork(skeletonRequests, exportDirectory, sessionStem, reporter);
        }
    );
}

} // namespace ovtr::win32
