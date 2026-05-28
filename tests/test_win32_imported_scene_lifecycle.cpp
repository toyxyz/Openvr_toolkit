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

void testWin32ImportedSceneLifecycle()
{
    ovtr::win32::AppImportedSceneState state;
    state.importedSceneLoaded = true;
    state.importedScene.durationSeconds = 2.0;

    const std::chrono::steady_clock::time_point base{std::chrono::seconds(10)};
    state.importedScenePlaybackSeconds = 2.0;
    require(
        ovtr::win32::startImportedScenePlaybackForRecording(state, base),
        "recording auto-start starts imported playback"
    );
    require(state.importedScenePlaying, "recording auto-start marks imported scene playing");
    require(nearDouble(state.importedScenePlaybackSeconds, 0.0), "recording auto-start rewinds from end");

    state.importedScene.durationSeconds = 0.0;
    state.importedScenePlaying = true;
    require(
        !ovtr::win32::startImportedScenePlaybackForRecording(state, base),
        "recording auto-start ignores zero-duration scene"
    );
    require(!state.importedScenePlaying, "zero-duration auto-start stops playback");

    state.importedSceneLoaded = true;
    state.importedScene.sourcePath = "C:/captures/scene.glb";
    state.importedScenePlaying = true;
    state.importedSceneTimelineDragging = true;
    state.importedScenePlaybackSeconds = 1.0;
    require(ovtr::win32::closeImportedScene(state), "close imported scene succeeds");
    require(!state.importedSceneLoaded, "close imported scene clears loaded flag");
    require(!state.importedScenePlaying, "close imported scene stops playback");
    require(!state.importedSceneTimelineDragging, "close imported scene clears drag state");
    require(nearDouble(state.importedScenePlaybackSeconds, 0.0), "close imported scene clears playback time");
    require(state.importStatusMessage == "GLB import closed: scene.glb", "close imported scene status");
}

} // namespace ovtr::test
