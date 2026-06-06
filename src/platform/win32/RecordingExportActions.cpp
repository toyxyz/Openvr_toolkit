#include "platform/win32/RecordingUiActions.h"

#include "platform/win32/AppLog.h"
#include "platform/win32/AppState.h"
#include "platform/win32/ExportProgressWorker.h"
#include "platform/win32/RecordingExportDispatch.h"
#include "platform/win32/RecordingExportMessages.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/RecordingStateQueries.h"
#include "platform/win32/SessionExportActions.h"
#include "platform/win32/SessionSkeletonExportBatch.h"
#include "platform/win32/SessionSkeletonExportClip.h"
#include "platform/win32/SkeletonGltfExporter.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <filesystem>
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

ExportProgressResult currentSessionExportWork(
    const RecordingExportPlan& plan,
    const std::vector<SessionSkeletonClipRequest>& skeletonRequests,
    const ExportProgressReporter& reporter
) {
    reporter.update(0.10f, "Writing GLB");
    const ovtr::ExportResult result = exportRecordingSession(
        plan.session,
        plan.exportDirectory,
        plan.exportSampleRate,
        plan.staticTracks
    );
    if (!result.success) {
        return progressResult(recordingExportFailureUiMessages(result.error));
    }

    RecordingExportUiMessages messages = recordingExportSuccessUiMessages(
        result.outputPath,
        true,
        {}
    );
    if (skeletonRequests.empty()) {
        messages.logMessages.push_back("Skeleton GLB export skipped: no calibrated Mapping actor");
        reporter.update(1.0f, "Export complete");
        return progressResult(std::move(messages));
    }

    exportSkeletonGlbsForActors(
        skeletonRequests,
        plan.exportDirectory,
        result.outputPath.stem().string(),
        reporter,
        messages
    );
    reporter.update(1.0f, "Export complete");
    return progressResult(std::move(messages));
}

} // namespace

void exportCurrentSession(HWND hwnd)
{
    AppWindowState* state = appStateForWindow(hwnd);
    if (!state) {
        return;
    }

    state->exportStatusMessage.clear();

    const std::string blockReason = exportBlockReason(*state);
    if (!blockReason.empty()) {
        state->exportStatusMessage = blockReason;
        appendDebugLog(*state, "Export blocked: " + state->exportStatusMessage);
        invalidateStatusPanel(hwnd);
        return;
    }

    RecordingExportPlan plan = makeRecordingExportPlan(
        static_cast<const AppRecordingState&>(*state),
        static_cast<const AppRuntimeState&>(*state),
        static_cast<const AppDeviceState&>(*state),
        static_cast<const AppMarkerState&>(*state),
        state->sessionName
    );
    const std::vector<SessionSkeletonClipRequest> skeletonRequests =
        sessionSkeletonClipRequests(*state, plan.session);

    if (isExportProgressActive(*state)) {
        state->exportStatusMessage = "export already in progress";
        appendDebugLog(*state, "Export blocked: export already in progress");
        invalidateStatusPanel(hwnd);
        return;
    }
    appendDebugLog(*state, recordingExportStartLogMessage());
    beginExportProgress(
        hwnd,
        *state,
        "Exporting GLB",
        [plan = std::move(plan),
         skeletonRequests](const ExportProgressReporter& reporter) {
            return currentSessionExportWork(
                plan,
                skeletonRequests,
                reporter
            );
        }
    );
}

} // namespace ovtr::win32
