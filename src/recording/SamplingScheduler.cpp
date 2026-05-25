#include "recording/SamplingScheduler.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ovtr {

SamplingScheduler::SamplingScheduler(const double targetFps)
    : targetFps_(targetFps)
{
    if (targetFps_ <= 0.0) {
        throw std::invalid_argument("target FPS must be greater than zero");
    }

    const auto intervalNs = static_cast<std::int64_t>(std::llround(1'000'000'000.0 / targetFps_));
    interval_ = std::chrono::nanoseconds(std::max<std::int64_t>(1, intervalNs));
}

void SamplingScheduler::reset(const std::chrono::steady_clock::time_point startTime)
{
    nextSampleTime_ = startTime;
    nextFrameIndex_ = 0;
    droppedFrameCount_ = 0;
    initialized_ = true;
}

bool SamplingScheduler::shouldSample(const std::chrono::steady_clock::time_point now) const
{
    return initialized_ && now >= nextSampleTime_;
}

SampleTiming SamplingScheduler::markSampled(const std::chrono::steady_clock::time_point now)
{
    if (!initialized_) {
        reset(now);
    }

    std::uint64_t intervalsToAdvance = 1;
    if (now > nextSampleTime_) {
        const auto lateBy = std::chrono::duration_cast<std::chrono::nanoseconds>(now - nextSampleTime_);
        intervalsToAdvance += static_cast<std::uint64_t>(lateBy.count() / interval_.count());
    }

    const std::uint64_t droppedThisSample = intervalsToAdvance > 1 ? intervalsToAdvance - 1 : 0;
    droppedFrameCount_ += droppedThisSample;

    const SampleTiming timing{nextFrameIndex_, droppedThisSample};
    ++nextFrameIndex_;
    nextSampleTime_ += interval_ * intervalsToAdvance;
    return timing;
}

double SamplingScheduler::targetFps() const
{
    return targetFps_;
}

std::uint64_t SamplingScheduler::nextFrameIndex() const
{
    return nextFrameIndex_;
}

std::uint64_t SamplingScheduler::droppedFrameCount() const
{
    return droppedFrameCount_;
}

std::chrono::nanoseconds SamplingScheduler::interval() const
{
    return interval_;
}

} // namespace ovtr

