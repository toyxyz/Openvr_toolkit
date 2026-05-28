#include "TestCases.h"
#include "TestSupport.h"

#include "recording/SamplingScheduler.h"

#include <chrono>

namespace ovtr::test {

void testSamplingScheduler()
{
    using clock = std::chrono::steady_clock;

    ovtr::SamplingScheduler scheduler(10.0);
    const auto start = clock::time_point{};
    scheduler.reset(start);

    require(scheduler.shouldSample(start), "scheduler should sample at start");
    const ovtr::SampleTiming first = scheduler.markSampled(start);
    require(first.frameIndex == 0, "first frame index should be zero");
    require(first.droppedFrames == 0, "first sample should not drop frames");
    require(!scheduler.shouldSample(start + std::chrono::milliseconds(50)), "scheduler sampled too early");
    require(scheduler.shouldSample(start + std::chrono::milliseconds(100)), "scheduler missed due sample");

    const ovtr::SampleTiming late = scheduler.markSampled(start + std::chrono::milliseconds(350));
    require(late.frameIndex == 1, "late sample frame index mismatch");
    require(late.droppedFrames == 2, "late sample should report two dropped frames");
    require(scheduler.droppedFrameCount() == 2, "scheduler cumulative dropped frame count mismatch");
}

} // namespace ovtr::test
