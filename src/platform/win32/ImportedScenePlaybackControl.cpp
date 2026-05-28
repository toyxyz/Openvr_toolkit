#include "platform/win32/ImportedScenePlayback.h"

#include "platform/win32/AppImportedSceneState.h"

namespace ovtr::win32 {

void updateImportedScenePlayback(
    AppImportedSceneState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    if (!state.importedSceneLoaded) {
        return;
    }

    if (state.importedSceneLastUpdate == std::chrono::steady_clock::time_point{}) {
        state.importedSceneLastUpdate = now;
    }
    if (!state.importedScenePlaying || state.importedSceneTimelineDragging) {
        state.importedSceneLastUpdate = now;
        return;
    }

    const double elapsedSeconds = std::chrono::duration<double>(now - state.importedSceneLastUpdate).count();
    state.importedSceneLastUpdate = now;
    const double durationSeconds = importedSceneDurationSeconds(state);
    state.importedScenePlaybackSeconds += elapsedSeconds > 0.0 ? elapsedSeconds : 0.0;
    if (state.importedScenePlaybackSeconds >= durationSeconds) {
        state.importedScenePlaybackSeconds = durationSeconds;
        state.importedScenePlaying = false;
    }
}

void updateImportedScenePlayback(AppImportedSceneState& state) noexcept
{
    updateImportedScenePlayback(state, std::chrono::steady_clock::now());
}

ImportedScenePlaybackToggleResult toggleImportedScenePlayback(
    AppImportedSceneState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    if (!state.importedSceneLoaded) {
        return ImportedScenePlaybackToggleResult::Ignored;
    }

    const double durationSeconds = importedSceneDurationSeconds(state);
    if (!state.importedScenePlaying && state.importedScenePlaybackSeconds >= durationSeconds) {
        setImportedScenePlaybackSeconds(state, 0.0, now);
    }
    state.importedScenePlaying = !state.importedScenePlaying;
    state.importedSceneLastUpdate = now;
    return state.importedScenePlaying
        ? ImportedScenePlaybackToggleResult::Started
        : ImportedScenePlaybackToggleResult::Paused;
}

ImportedScenePlaybackToggleResult toggleImportedScenePlayback(AppImportedSceneState& state) noexcept
{
    return toggleImportedScenePlayback(state, std::chrono::steady_clock::now());
}

} // namespace ovtr::win32
