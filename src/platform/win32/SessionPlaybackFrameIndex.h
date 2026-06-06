#pragma once

#include "platform/win32/AppLoadedSessionState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ovtr::win32 {

inline std::uint64_t loadedSessionFrameIndexForPlayback(const AppLoadedSessionState& state) noexcept
{
    const std::uint64_t frameCount = state.loadedSessionReader.frameCount();
    if (frameCount <= 1) {
        return 0;
    }
    const double duration = state.loadedSessionActive && state.loadedSessionDurationSeconds > 0.0
        ? state.loadedSessionDurationSeconds
        : 0.0;
    const double factor = duration > 0.0
        ? std::clamp(state.loadedSessionPlaybackSeconds / duration, 0.0, 1.0)
        : 0.0;
    return static_cast<std::uint64_t>(std::round(factor * static_cast<double>(frameCount - 1)));
}

} // namespace ovtr::win32
