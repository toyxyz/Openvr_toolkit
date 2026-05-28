#include "platform/win32/RuntimeStatusInternal.h"

#include "platform/win32/AppRuntimeState.h"
#include <chrono>
#include <cstdint>

namespace ovtr::win32 {

void updateFpsCounters(AppRuntimeState& state)
{
    const auto now = std::chrono::steady_clock::now();
    const double elapsedSeconds = std::chrono::duration<double>(now - state.lastFpsUpdate).count();
    if (elapsedSeconds < 1.0) {
        return;
    }

    const std::uint64_t posePollFrames = state.posePollFrames.exchange(0);
    state.posePollFps = static_cast<double>(posePollFrames) / elapsedSeconds;
    state.renderFps = static_cast<double>(state.renderFrames) / elapsedSeconds;
    state.renderFrames = 0;
    state.lastFpsUpdate = now;
}

} // namespace ovtr::win32
