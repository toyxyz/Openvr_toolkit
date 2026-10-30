#pragma once

#include "TestSupport.h"
#include "platform/win32/CombinedPlayback.h"
#include <cmath>

namespace ovtr::test {

inline void verifyCombinedPlayback(const std::filesystem::path& sessionFolder)
{
    using namespace ovtr::win32;
    using namespace std::chrono_literals;
    const auto start = ovtr::PlaybackTimeline::Clock::time_point{} + 10s;
    AppWindowState state;
    std::string error;
    require(openLoadedSession(state, sessionFolder, error), "combined playback session: " + error);
    state.importedSceneLoaded = true;
    state.importedScene.durationSeconds = 3.0;
    ovtr::ImportedGltfNode node;
    node.keys = {{0.0, {0.0f, 0.0f, 0.0f}}, {3.0, {3.0f, 0.0f, 0.0f}}};
    state.importedScene.nodes.push_back(node);
    reconcileCombinedPlayback(state, start);
    require(combinedPlaybackDuration(state) == 3.0, "longer GLB defines timeline duration");
    require(playbackTimelineView(state, state).totalFrames == 181, "longer GLB defines ruler frame range");
    toggleCombinedPlayback(state, start);
    updateCombinedPlayback(state, start + 500ms);
    require(state.loadedSessionPlaying && state.importedScenePlaying, "one transport starts both sources");
    require(state.loadedSessionPlaybackSeconds == .5 && state.importedScenePlaybackSeconds == .5,
        "clips run at the same absolute time without stretching");
    std::array<float, 3> translation{};
    std::array<float, 4> rotation{};
    require(ovtr::sampleImportedGltfNodePose(state.importedScene.nodes[0], state.importedScenePlaybackSeconds,
        translation, rotation) && std::abs(translation[0] - .5f) < 1e-6f,
        "shared time drives actual GLB animation sampling");
    updateCombinedPlayback(state, start + 2s);
    require(state.loadedSessionPlaybackSeconds == 1.0 && state.importedScenePlaybackSeconds == 2.0,
        "shorter session holds its end while GLB continues");
    require(state.playbackTimeline.playing, "shorter source end does not stop the common clock");
    require(sampleLoadedSessionFrame(state), "sample shorter session endpoint");
    require(state.poses.poses[0].position[0] == 2.0f, "session endpoint pose remains visible");
    require(state.loadedSessionLastSampledFrameValid, "session endpoint cached");
    updateCombinedPlayback(state, start + 2500ms);
    require(state.loadedSessionLastSampledFrameValid, "holding endpoint does not invalidate pose cache every tick");
    toggleCombinedPlayback(state, start + 2500ms);
    updateCombinedPlayback(state, start + 5s);
    require(state.playbackTimeline.seconds == 2.5 && !state.loadedSessionPlaying && !state.importedScenePlaying,
        "pause stops both clocks");
    require(seekCombinedPlaybackFromTimeline(state, RECT{0, 0, 300, 20}, POINT{150, 10}),
        "common timeline accepts seek");
    require(state.playbackTimeline.seconds == 1.5 && state.loadedSessionPlaybackSeconds == 1.0 &&
        state.importedScenePlaybackSeconds == 1.5, "seek uses full longer-source range");
    require(!state.loadedSessionLastSampledFrameValid, "explicit seek invalidates session cache");
    seekCombinedPlayback(state, 3.0, start + 6s);
    toggleCombinedPlayback(state, start + 6s);
    require(state.playbackTimeline.seconds == 0.0 && state.playbackTimeline.playing, "play at end restarts both");
    state.loadedSessionTimelineDragging = true;
    updateCombinedPlayback(state, start + 7s);
    require(state.playbackTimeline.seconds == 0.0, "dragging suspends common clock advancement");
    state.loadedSessionTimelineDragging = false;
    updateCombinedPlayback(state, start + 7500ms);
    require(state.playbackTimeline.seconds == .5, "drag release resumes without accumulated elapsed time");
    updateCombinedPlayback(state, start + 11s);
    require(state.playbackTimeline.seconds == 3.0 && !state.loadedSessionPlaying && !state.importedScenePlaying,
        "longest source end stops both");

    state.importedScene.durationSeconds = .25;
    state.importedScene.nodes[0].keys.back().timeSeconds = .25;
    reconcileCombinedPlayback(state, start + 12s);
    require(combinedPlaybackDuration(state) == 1.0, "longer session defines timeline duration");
    require(playbackTimelineView(state, state).totalFrames == 2, "session ruler uses recorded frame count");
    seekCombinedPlayback(state, 0.0, start + 12s);
    toggleCombinedPlayback(state, start + 12s);
    updateCombinedPlayback(state, start + 12750ms);
    require(state.loadedSessionPlaybackSeconds == .75 && state.importedScenePlaybackSeconds == .25,
        "shorter GLB holds its last pose while the session continues");
    require(ovtr::sampleImportedGltfNodePose(state.importedScene.nodes[0], state.importedScenePlaybackSeconds,
        translation, rotation) && translation[0] == 3.0f, "shorter GLB samples its final key");
    require(state.importedScenePlaying && state.loadedSessionPlaying, "both flags reflect common transport");

    state.importedSceneLoaded = false;
    reconcileCombinedPlayback(state, start + 13s);
    require(state.loadedSessionPlaybackSeconds == .75 && state.loadedSessionPlaying,
        "closing GLB preserves surviving session position and playback");
    state.importedSceneLoaded = true;
    state.importedScene.durationSeconds = 4.0;
    reconcileCombinedPlayback(state, start + 14s);
    require(state.importedScenePlaybackSeconds == .75 && state.importedScenePlaying,
        "new GLB joins the current session time");
    require(closeLoadedSession(state), "close session independently");
    reconcileCombinedPlayback(state, start + 15s);
    require(state.importedScenePlaybackSeconds == .75 && state.importedScenePlaying,
        "closing session preserves GLB playback");
    state.importedSceneLoaded = false;
    reconcileCombinedPlayback(state, start + 16s);
    require(state.playbackTimeline.seconds == 0.0 && !state.playbackTimeline.playing,
        "closing all sources clears common timeline");

    state.importedSceneLoaded = true;
    state.importedScene.durationSeconds = 4.0;
    reconcileCombinedPlayback(state, start + 17s);
    seekCombinedPlayback(state, .6, start + 17s);
    toggleCombinedPlayback(state, start + 17s);
    require(openLoadedSession(state, sessionFolder, error), "open session after GLB: " + error);
    reconcileCombinedPlayback(state, start + 18s);
    require(std::abs(state.loadedSessionPlaybackSeconds - .6) < 1e-9 && state.loadedSessionPlaying,
        "new session joins existing GLB time and transport");
    closeLoadedSession(state);
}

} // namespace ovtr::test
