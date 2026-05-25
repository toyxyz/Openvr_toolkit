#pragma once

#include <chrono>
#include <cstdint>

namespace ovtr {

struct SampleTiming {
    std::uint64_t frameIndex = 0;
    std::uint64_t droppedFrames = 0;
};

class SamplingScheduler {
public:
    explicit SamplingScheduler(double targetFps = 90.0);

    void reset(std::chrono::steady_clock::time_point startTime);
    bool shouldSample(std::chrono::steady_clock::time_point now) const;
    SampleTiming markSampled(std::chrono::steady_clock::time_point now);

    double targetFps() const;
    std::uint64_t nextFrameIndex() const;
    std::uint64_t droppedFrameCount() const;
    std::chrono::nanoseconds interval() const;

private:
    double targetFps_ = 90.0;
    std::chrono::nanoseconds interval_{};
    std::chrono::steady_clock::time_point nextSampleTime_{};
    std::uint64_t nextFrameIndex_ = 0;
    std::uint64_t droppedFrameCount_ = 0;
    bool initialized_ = false;
};

} // namespace ovtr

