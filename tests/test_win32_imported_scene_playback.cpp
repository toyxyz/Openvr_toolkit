#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppImportedSceneState.h"
#include "platform/win32/ImportedScenePlayback.h"

#include <chrono>
#include <cmath>

namespace {

bool nearDouble(const double actual, const double expected)
{
    return std::fabs(actual - expected) < 0.000001;
}

} // namespace

namespace ovtr::test {

void testWin32ImportedScenePlayback()
{
    ovtr::win32::AppImportedSceneState state;
    require(
        nearDouble(ovtr::win32::importedSceneDurationSeconds(state), 0.0),
        "unloaded imported scene has no duration"
    );
    require(ovtr::win32::importedSceneTotalFrames(state) == 0, "unloaded imported scene has no frames");
    require(ovtr::win32::importedSceneCurrentFrame(state) == 0, "unloaded imported scene has no current frame");

    state.importedSceneLoaded = true;
    state.importedScene.durationSeconds = 1.0;
    require(
        nearDouble(ovtr::win32::importedSceneDurationSeconds(state), 1.0),
        "imported scene duration"
    );
    require(ovtr::win32::importedSceneTotalFrames(state) == 61, "imported scene total frames");
    require(ovtr::win32::importedSceneCurrentFrame(state) == 1, "imported scene first frame");

    const std::chrono::steady_clock::time_point base{std::chrono::seconds(10)};
    ovtr::win32::setImportedScenePlaybackSeconds(state, 0.5, base);
    require(nearDouble(state.importedScenePlaybackSeconds, 0.5), "set imported scene playback time");
    require(state.importedSceneLastUpdate == base, "set imported scene update timestamp");
    require(ovtr::win32::importedSceneCurrentFrame(state) == 31, "imported scene middle frame");

    ovtr::win32::setImportedScenePlaybackSeconds(state, 2.0, base);
    require(nearDouble(state.importedScenePlaybackSeconds, 1.0), "set imported scene clamps high");
    require(ovtr::win32::importedSceneCurrentFrame(state) == 61, "imported scene final frame");

    ovtr::win32::setImportedScenePlaybackSeconds(state, -1.0, base);
    require(nearDouble(state.importedScenePlaybackSeconds, 0.0), "set imported scene clamps low");

    state.importedScene.durationSeconds = 2.0;
    state.importedScenePlaybackSeconds = 0.25;
    state.importedScenePlaying = true;
    state.importedSceneTimelineDragging = false;
    state.importedSceneLastUpdate = base;
    ovtr::win32::updateImportedScenePlayback(state, base + std::chrono::milliseconds(500));
    require(nearDouble(state.importedScenePlaybackSeconds, 0.75), "update imported scene advances playback");
    require(state.importedScenePlaying, "update imported scene keeps playback active before end");

    ovtr::win32::updateImportedScenePlayback(state, base + std::chrono::seconds(4));
    require(nearDouble(state.importedScenePlaybackSeconds, 2.0), "update imported scene clamps at end");
    require(!state.importedScenePlaying, "update imported scene stops at end");

    state.importedScenePlaybackSeconds = 0.5;
    state.importedScenePlaying = true;
    state.importedSceneTimelineDragging = true;
    ovtr::win32::updateImportedScenePlayback(state, base + std::chrono::seconds(5));
    require(nearDouble(state.importedScenePlaybackSeconds, 0.5), "dragging imported timeline pauses advance");

    state.importedSceneTimelineDragging = false;
    state.importedScene.durationSeconds = 2.0;
    require(
        ovtr::win32::seekImportedSceneFromTimeline(state, RECT{10, 0, 110, 20}, POINT{60, 5}),
        "timeline seek succeeds for loaded scene"
    );
    require(nearDouble(state.importedScenePlaybackSeconds, 1.0), "timeline seek maps point to playback time");
    require(
        !ovtr::win32::seekImportedSceneFromTimeline(state, RECT{10, 0, 10, 20}, POINT{60, 5}),
        "timeline seek rejects invalid rect"
    );

    state.importedScenePlaybackSeconds = 2.0;
    state.importedScenePlaying = false;
    const ovtr::win32::ImportedScenePlaybackToggleResult startResult =
        ovtr::win32::toggleImportedScenePlayback(state, base);
    require(
        startResult == ovtr::win32::ImportedScenePlaybackToggleResult::Started,
        "toggle starts imported playback"
    );
    require(state.importedScenePlaying, "toggle marks imported scene playing");
    require(nearDouble(state.importedScenePlaybackSeconds, 0.0), "toggle restarts from end");
    const ovtr::win32::ImportedScenePlaybackToggleResult pauseResult =
        ovtr::win32::toggleImportedScenePlayback(state, base + std::chrono::milliseconds(10));
    require(
        pauseResult == ovtr::win32::ImportedScenePlaybackToggleResult::Paused,
        "toggle pauses imported playback"
    );
    require(!state.importedScenePlaying, "toggle marks imported scene paused");

}

} // namespace ovtr::test
