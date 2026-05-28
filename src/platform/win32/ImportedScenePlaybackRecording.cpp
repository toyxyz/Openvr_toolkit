#include "platform/win32/ImportedScenePlayback.h"

#include "platform/win32/AppImportedSceneState.h"

namespace ovtr::win32 {

bool startImportedScenePlaybackForRecording(
    AppImportedSceneState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    if (!state.importedSceneLoaded) {
        return false;
    }

    const double durationSeconds = importedSceneDurationSeconds(state);
    if (durationSeconds <= 0.0) {
        state.importedScenePlaying = false;
        setImportedScenePlaybackSeconds(state, 0.0, now);
        return false;
    }

    if (state.importedScenePlaybackSeconds >= durationSeconds) {
        setImportedScenePlaybackSeconds(state, 0.0, now);
    } else {
        state.importedSceneLastUpdate = now;
    }
    state.importedScenePlaying = true;
    return true;
}

bool startImportedScenePlaybackForRecording(AppImportedSceneState& state) noexcept
{
    return startImportedScenePlaybackForRecording(state, std::chrono::steady_clock::now());
}

} // namespace ovtr::win32
