#pragma once

#include "platform/win32/AppState.h"
#include "platform/win32/PlaybackTimelineView.h"

namespace ovtr::win32 {

inline double combinedPlaybackDuration(const AppWindowState& state) noexcept
{
    return playbackTimelineView(state, state).duration;
}

inline void applyCombinedPlayback(AppWindowState& state, const bool explicitSeek,
    const PlaybackTimeline::Clock::time_point now) noexcept
{
    const auto& timeline = state.playbackTimeline;
    state.importedScenePlaybackSeconds = std::clamp(timeline.seconds, 0.0, importedSceneDurationSeconds(state));
    state.loadedSessionPlaybackSeconds = std::clamp(timeline.seconds, 0.0, loadedSessionDurationSeconds(state));
    state.importedScenePlaying = state.importedSceneLoaded && timeline.playing;
    state.loadedSessionPlaying = state.loadedSessionActive && timeline.playing;
    state.importedSceneLastUpdate = state.loadedSessionLastUpdate = now;
    if (explicitSeek) {
        state.loadedSessionLastSampledFrameValid = false;
        state.viewportRenderDemand.dirty = true;
    }
}

inline void reconcileCombinedPlayback(AppWindowState& state,
    const PlaybackTimeline::Clock::time_point now = PlaybackTimeline::Clock::now()) noexcept
{
    if (!state.importedSceneLoaded && !state.loadedSessionActive) state.playbackTimeline = {};
    state.playbackTimeline.seek(state.playbackTimeline.seconds, combinedPlaybackDuration(state), now);
    applyCombinedPlayback(state, true, now);
}

inline void seekCombinedPlayback(AppWindowState& state, const double seconds,
    const PlaybackTimeline::Clock::time_point now = PlaybackTimeline::Clock::now()) noexcept
{
    state.playbackTimeline.seek(seconds, combinedPlaybackDuration(state), now);
    applyCombinedPlayback(state, true, now);
}

inline void toggleCombinedPlayback(AppWindowState& state,
    const PlaybackTimeline::Clock::time_point now = PlaybackTimeline::Clock::now()) noexcept
{
    const bool rewinding = !state.playbackTimeline.playing &&
        state.playbackTimeline.seconds >= combinedPlaybackDuration(state);
    state.playbackTimeline.toggle(combinedPlaybackDuration(state), now);
    applyCombinedPlayback(state, rewinding, now);
}

inline void updateCombinedPlayback(AppWindowState& state,
    const PlaybackTimeline::Clock::time_point now = PlaybackTimeline::Clock::now()) noexcept
{
    state.playbackTimeline.update(combinedPlaybackDuration(state),
        state.loadedSessionTimelineDragging || state.importedSceneTimelineDragging, now);
    applyCombinedPlayback(state, false, now);
}

inline bool seekCombinedPlaybackFromTimeline(AppWindowState& state, const RECT& rect, const POINT point) noexcept
{
    if ((!state.importedSceneLoaded && !state.loadedSessionActive) || rect.right <= rect.left) return false;
    const double factor = static_cast<double>(std::clamp(point.x, rect.left, rect.right) - rect.left) /
        static_cast<double>(rect.right - rect.left);
    seekCombinedPlayback(state, factor * combinedPlaybackDuration(state));
    return true;
}

} // namespace ovtr::win32
