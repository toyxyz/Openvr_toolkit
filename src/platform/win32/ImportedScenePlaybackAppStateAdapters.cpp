#include "platform/win32/ImportedScenePlayback.h"

#include "platform/win32/AppState.h"

namespace ovtr::win32 {

double importedSceneDurationSeconds(const AppWindowState& state) noexcept
{
    return importedSceneDurationSeconds(static_cast<const AppImportedSceneState&>(state));
}

int importedSceneTotalFrames(const AppWindowState& state) noexcept
{
    return importedSceneTotalFrames(static_cast<const AppImportedSceneState&>(state));
}

int importedSceneCurrentFrame(const AppWindowState& state) noexcept
{
    return importedSceneCurrentFrame(static_cast<const AppImportedSceneState&>(state));
}

void setImportedScenePlaybackSeconds(
    AppWindowState& state,
    const double playbackSeconds,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    setImportedScenePlaybackSeconds(static_cast<AppImportedSceneState&>(state), playbackSeconds, now);
}

void setImportedScenePlaybackSeconds(AppWindowState& state, const double playbackSeconds) noexcept
{
    setImportedScenePlaybackSeconds(static_cast<AppImportedSceneState&>(state), playbackSeconds);
}

void updateImportedScenePlayback(
    AppWindowState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    updateImportedScenePlayback(static_cast<AppImportedSceneState&>(state), now);
}

void updateImportedScenePlayback(AppWindowState& state) noexcept
{
    updateImportedScenePlayback(static_cast<AppImportedSceneState&>(state));
}

double importedScenePlaybackTime(const AppWindowState& state) noexcept
{
    return importedScenePlaybackTime(static_cast<const AppImportedSceneState&>(state));
}

bool closeImportedScene(AppWindowState& state)
{
    return closeImportedScene(static_cast<AppImportedSceneState&>(state));
}

bool seekImportedSceneFromTimeline(
    AppWindowState& state,
    const RECT& timelineRect,
    const POINT point
) noexcept
{
    return seekImportedSceneFromTimeline(static_cast<AppImportedSceneState&>(state), timelineRect, point);
}

ImportedScenePlaybackToggleResult toggleImportedScenePlayback(
    AppWindowState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    return toggleImportedScenePlayback(static_cast<AppImportedSceneState&>(state), now);
}

ImportedScenePlaybackToggleResult toggleImportedScenePlayback(AppWindowState& state) noexcept
{
    return toggleImportedScenePlayback(static_cast<AppImportedSceneState&>(state));
}

bool startImportedScenePlaybackForRecording(
    AppWindowState& state,
    const std::chrono::steady_clock::time_point now
) noexcept
{
    return startImportedScenePlaybackForRecording(static_cast<AppImportedSceneState&>(state), now);
}

bool startImportedScenePlaybackForRecording(AppWindowState& state) noexcept
{
    return startImportedScenePlaybackForRecording(static_cast<AppImportedSceneState&>(state));
}

} // namespace ovtr::win32
