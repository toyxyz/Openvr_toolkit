#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppRuntimeState.h"
#include "platform/win32/RuntimeStatusInternal.h"

#include <chrono>
#include <cmath>

namespace ovtr::test {

void testWin32RuntimeStatus()
{
    ovtr::win32::AppRuntimeState state;
    state.posePollFrames = 24;
    state.renderFrames = 12;
    state.lastFpsUpdate = std::chrono::steady_clock::now() - std::chrono::seconds(2);

    ovtr::win32::updateFpsCounters(state);

    require(std::abs(state.posePollFps - 12.0) < 0.25, "pose FPS counter updates from runtime state");
    require(std::abs(state.renderFps - 6.0) < 0.25, "render FPS counter updates from runtime state");
    require(state.posePollFrames == 0, "pose frame counter resets");
    require(state.renderFrames == 0, "render frame counter resets");

    state.posePollFps = 7.0;
    state.renderFps = 3.0;
    state.posePollFrames = 100;
    state.renderFrames = 50;
    state.lastFpsUpdate = std::chrono::steady_clock::now();

    ovtr::win32::updateFpsCounters(state);

    require(state.posePollFps == 7.0, "pose FPS remains unchanged before interval");
    require(state.renderFps == 3.0, "render FPS remains unchanged before interval");
    require(state.posePollFrames == 100, "pose frame counter waits for interval");
    require(state.renderFrames == 50, "render frame counter waits for interval");
}

} // namespace ovtr::test
