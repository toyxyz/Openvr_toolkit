#include "recording/RecordingController.h"

#include <utility>

namespace ovtr {

bool RecordingController::writeManifest(
    const bool finalized,
    const double durationSeconds,
    const std::uint64_t droppedFrames
)
{
    SessionManifestStats stats;
    stats.frameCount = frameCount_;
    stats.durationSeconds = durationSeconds;
    stats.droppedFrames = droppedFrames;
    stats.finalized = finalized;

    std::string error;
    if (!writeManifestJson(session_, stats, sessionFolder_ / "manifest.json", error)) {
        setError(error);
        return false;
    }

    return true;
}

void RecordingController::setError(std::string message)
{
    lastError_ = std::move(message);
    state_ = RecorderState::Error;
}

} // namespace ovtr
