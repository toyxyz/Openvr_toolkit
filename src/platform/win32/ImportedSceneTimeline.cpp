#include "platform/win32/ImportedScenePlayback.h"

#include "platform/win32/AppImportedSceneState.h"

#include <algorithm>
#include <cmath>

namespace ovtr::win32 {

double importedSceneDurationSeconds(const AppImportedSceneState& state) noexcept
{
    if (!state.importedSceneLoaded || state.importedScene.durationSeconds <= 0.000001) {
        return 0.0;
    }
    return state.importedScene.durationSeconds;
}

int importedSceneTotalFrames(const AppImportedSceneState& state) noexcept
{
    const double durationSeconds = importedSceneDurationSeconds(state);
    if (durationSeconds <= 0.0) {
        return state.importedSceneLoaded ? 1 : 0;
    }
    const int totalFrames = static_cast<int>(std::floor(durationSeconds * kImportedAnimationFrameRate)) + 1;
    return totalFrames > 1 ? totalFrames : 1;
}

int importedSceneCurrentFrame(const AppImportedSceneState& state) noexcept
{
    const int totalFrames = importedSceneTotalFrames(state);
    if (totalFrames <= 0) {
        return 0;
    }

    const int frame = static_cast<int>(std::floor(
        std::clamp(state.importedScenePlaybackSeconds, 0.0, importedSceneDurationSeconds(state)) *
        kImportedAnimationFrameRate
    )) + 1;
    return std::clamp(frame, 1, totalFrames);
}

double importedScenePlaybackTime(const AppImportedSceneState& state) noexcept
{
    return state.importedSceneLoaded
        ? std::clamp(state.importedScenePlaybackSeconds, 0.0, importedSceneDurationSeconds(state))
        : 0.0;
}

bool seekImportedSceneFromTimeline(
    AppImportedSceneState& state,
    const RECT& timelineRect,
    const POINT point
) noexcept
{
    if (!state.importedSceneLoaded || timelineRect.right <= timelineRect.left) {
        return false;
    }

    const int clampedX = std::clamp(point.x, timelineRect.left, timelineRect.right);
    const double factor = static_cast<double>(clampedX - timelineRect.left) /
        static_cast<double>(timelineRect.right - timelineRect.left);
    setImportedScenePlaybackSeconds(state, factor * importedSceneDurationSeconds(state));
    return true;
}

} // namespace ovtr::win32
