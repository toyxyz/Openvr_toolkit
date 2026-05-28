#include "platform/win32/ImportedScenePlayback.h"

#include "platform/win32/AppImportedSceneState.h"

#include <algorithm>

namespace ovtr::win32 {

void setImportedScenePlaybackSeconds(
    AppImportedSceneState& state,
    const double playbackSeconds,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    const double durationSeconds = importedSceneDurationSeconds(state);
    state.importedScenePlaybackSeconds = std::clamp(playbackSeconds, 0.0, durationSeconds);
    state.importedSceneLastUpdate = now;
}

void setImportedScenePlaybackSeconds(AppImportedSceneState& state, const double playbackSeconds) noexcept
{
    setImportedScenePlaybackSeconds(state, playbackSeconds, std::chrono::steady_clock::now());
}

} // namespace ovtr::win32
