#pragma once

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/AppLoadedSessionState.h"
#include "platform/win32/ImportedScenePlayback.h"
#include "platform/win32/SessionPlayback.h"
#include <algorithm>

namespace ovtr::win32 {

struct PlaybackTimelineView {
    double duration = 0.0;
    double seconds = 0.0;
    int currentFrame = 0;
    int totalFrames = 0;
};

inline PlaybackTimelineView playbackTimelineView(const AppImportedSceneState& glb,
    const AppLoadedSessionState& session) noexcept
{
    const double glbDuration = importedSceneDurationSeconds(glb);
    const double sessionDuration = loadedSessionDurationSeconds(session);
    if (session.loadedSessionActive && (!glb.importedSceneLoaded || sessionDuration >= glbDuration)) {
        return {sessionDuration, session.loadedSessionPlaybackSeconds,
            loadedSessionCurrentFrame(session), loadedSessionTotalFrames(session)};
    }
    return {glbDuration, glb.importedScenePlaybackSeconds,
        importedSceneCurrentFrame(glb), importedSceneTotalFrames(glb)};
}

} // namespace ovtr::win32
