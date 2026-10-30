#pragma once

#include <algorithm>
#include <chrono>

namespace ovtr {

// A single clock in seconds; source clips clamp sampling to their own end.
struct PlaybackTimeline {
    using Clock = std::chrono::steady_clock;
    double seconds = 0.0;
    bool playing = false;
    Clock::time_point lastUpdate{};

    void seek(double requested, double duration, Clock::time_point now) noexcept
    {
        seconds = std::clamp(requested, 0.0, duration);
        lastUpdate = now;
        if (duration <= 0.0 || seconds >= duration) playing = false;
    }
    void update(double duration, bool dragging, Clock::time_point now) noexcept
    {
        if (lastUpdate != Clock::time_point{} && playing && !dragging) {
            seconds += (std::max)(0.0, std::chrono::duration<double>(now - lastUpdate).count());
        }
        seek(seconds, duration, now);
    }
    void toggle(double duration, Clock::time_point now) noexcept
    {
        if (duration <= 0.0) return;
        if (!playing && seconds >= duration) seconds = 0.0;
        playing = !playing;
        lastUpdate = now;
    }
};

} // namespace ovtr
