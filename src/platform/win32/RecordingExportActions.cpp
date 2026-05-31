#include "platform/win32/RecordingUiActions.h"

#include "platform/win32/AppState.h"
#include "platform/win32/AppLog.h"
#include "platform/win32/RecordingCleanupActions.h"
#include "platform/win32/RecordingExportDispatch.h"
#include "platform/win32/RecordingExportMessages.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/RecordingStateQueries.h"
#include "platform/win32/SkeletonBvhExporter.h"
#include "platform/win32/SkeletonRecording.h"
#include "platform/win32/WindowLayout.h"
#include "platform/win32/WindowStateAccess.h"

#include <filesystem>

namespace ovtr::win32 {

void exportCurrentSession(HWND hwnd, const ExportFormat format)
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

    const RecordingExportPlan plan = makeRecordingExportPlan(
        static_cast<const AppRecordingState&>(*state),
        static_cast<const AppRuntimeState&>(*state),
        static_cast<const AppDeviceState&>(*state),
        static_cast<const AppMarkerState&>(*state),
        state->sessionName
    );
    appendDebugLog(*state, recordingExportStartLogMessage(format));
    const bool shouldExportSkeletonBvh = hasSkeletonRecordingFrames(state->skeletonRecording);
    const ovtr::ExportResult result = exportRecordingSession(
        plan.session,
        format,
        plan.exportDirectory,
        plan.exportSampleRate,
        plan.staticTracks
    );

    if (result.success) {
        std::string bvhError;
        const std::filesystem::path bvhPath =
            plan.exportDirectory / (plan.session.sessionId + "_skeleton.bvh");
        const bool bvhSucceeded = shouldExportSkeletonBvh &&
            exportSkeletonRecordingToBvh(state->skeletonRecording, bvhPath, bvhError);

        std::string cleanupMessage;
        const bool cleanupSucceeded = deleteTemporarySessionFolder(
            state->currentSessionFolder,
            std::filesystem::current_path() / "recordings",
            cleanupMessage
        );
        const RecordingExportUiMessages messages = recordingExportSuccessUiMessages(
            format,
            result.outputPath,
            cleanupSucceeded,
            cleanupMessage
        );
        state->exportStatusMessage = messages.statusMessage;
        for (const std::string& message : messages.logMessages) {
            appendDebugLog(*state, message);
        }
        if (bvhSucceeded) {
            appendDebugLog(*state, "Skeleton BVH export saved: " + bvhPath.string());
        } else if (shouldExportSkeletonBvh) {
            appendDebugLog(*state, "Skeleton BVH export failed: " + bvhError);
            state->exportStatusMessage += " Skeleton BVH export failed.";
        } else {
            appendDebugLog(*state, "Skeleton BVH export skipped: no skeleton frames");
        }
    } else {
        const RecordingExportUiMessages messages = recordingExportFailureUiMessages(format, result.error);
        state->exportStatusMessage = messages.statusMessage;
        for (const std::string& message : messages.logMessages) {
            appendDebugLog(*state, message);
        }
    }

    invalidateStatusPanel(hwnd);
}

} // namespace ovtr::win32
