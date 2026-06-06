#include "platform/win32/SessionSkeletonExportBatch.h"

#include "platform/win32/RecordingExportDispatch.h"
#include "platform/win32/RecordingExportPlan.h"
#include "platform/win32/SkeletonGltfExporter.h"
#include "platform/win32/Win32String.h"

#include <cstddef>

namespace ovtr::win32 {
namespace {

std::string actorFileStem(const SessionSkeletonClipRequest& request)
{
    std::wstring stem = sanitizedSessionFolderName(request.actor.profile.name);
    if (stem.empty()) {
        stem = L"actor_" + std::to_wstring(request.actor.id);
    }
    return narrow(stem);
}

void appendSkeletonFailure(RecordingExportUiMessages& messages, const std::string& error)
{
    messages.logMessages.push_back("Skeleton GLB export failed: " + error);
    messages.statusMessage += " Skeleton GLB export failed.";
}

} // namespace

void exportSkeletonGlbsForActors(
    const std::vector<SessionSkeletonClipRequest>& requests,
    const std::filesystem::path& exportDirectory,
    const std::string& sessionStem,
    const ExportProgressReporter& reporter,
    RecordingExportUiMessages& messages
) {
    if (requests.empty()) {
        messages.logMessages.push_back("Skeleton GLB export skipped: no calibrated Mapping actor");
        return;
    }

    const float step = 0.18f / static_cast<float>(requests.size());
    for (std::size_t index = 0; index < requests.size(); ++index) {
        reporter.update(0.68f + step * static_cast<float>(index), "Solving skeleton frames");
        SkeletonRecordingClip clip;
        std::string skeletonError;
        std::vector<std::string> skeletonWarnings;
        const bool builtClip = buildSessionSkeletonClipFromRequest(
            requests[index],
            clip,
            skeletonError,
            &skeletonWarnings
        );
        for (const std::string& warning : skeletonWarnings) {
            messages.logMessages.push_back(warning);
        }

        const std::filesystem::path skeletonPath = uniqueExportOutputPath(
            exportDirectory,
            sessionStem + "_skeleton_" + actorFileStem(requests[index]),
            ".glb"
        );
        reporter.update(0.86f, "Writing skeleton GLB");
        if (builtClip && exportSkeletonRecordingToGlb(clip, skeletonPath, skeletonError)) {
            messages.logMessages.push_back("Skeleton GLB export saved: " + skeletonPath.string());
        } else {
            appendSkeletonFailure(messages, skeletonError);
        }
    }
}

} // namespace ovtr::win32
