#pragma once

#include <chrono>

namespace ovtr {

using FrameClock = std::chrono::steady_clock;
inline constexpr auto kViewportFrameInterval =
    std::chrono::duration_cast<FrameClock::duration>(std::chrono::duration<double>(1.0 / 90.0));
inline constexpr auto kMessageDispatchBudget = std::chrono::milliseconds(2);

inline bool shouldYieldMessages(const FrameClock::time_point now,
    const FrameClock::time_point batchStarted, const FrameClock::time_point nextFrame) noexcept
{
    return now >= nextFrame || now - batchStarted >= kMessageDispatchBudget;
}

inline FrameClock::time_point nextViewportFrame(const FrameClock::time_point started,
    const FrameClock::time_point finished) noexcept
{
    const auto target = started + kViewportFrameInterval;
    // A slow frame must not leave an accumulating backlog or introduce another wait.
    return finished > target ? finished : target;
}

} // namespace ovtr
