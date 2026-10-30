#include "TestCases.h"
#include "TestSupport.h"

#include "recording/SamplingScheduler.h"
#include "app/FrameLoopSchedule.h"
#include "app/ViewportRenderDemand.h"

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

    using namespace std::chrono_literals;
    require(!ovtr::shouldYieldMessages(start + 1ms, start, start + 11ms),
        "short input batches can continue until a frame is due");
    require(ovtr::shouldYieldMessages(start + 2ms, start, start + 11ms),
        "continuous mouse input must yield after its dispatch budget");
    require(ovtr::shouldYieldMessages(start + 11ms, start + 10ms, start + 11ms),
        "a due frame takes priority over queued input");
    require(ovtr::nextViewportFrame(start, start + 3ms) == start + ovtr::kViewportFrameInterval,
        "light frames retain the display frame limit");
    require(ovtr::nextViewportFrame(start, start + 40ms) == start + 40ms,
        "slow frames neither accumulate catch-up deadlines nor impose an extra wait");

    ovtr::ViewportRenderDemand demand;
    require(demand.needsRender(true, 10, 0, -1.0, false), "new viewport requires a first frame");
    demand.presented(10, 0, -1.0);
    require(!demand.needsRender(true, 20, 0, -1.0, false),
        "empty live snapshots must not continuously redraw a static viewport");
    require(demand.needsRender(true, 20, 1, -1.0, false), "device arrival requires redraw");
    demand.presented(20, 1, -1.0);
    require(!demand.needsRender(true, 20, 1, -1.0, false), "paused frame can remain displayed");
    require(demand.needsRender(true, 21, 1, -1.0, false), "new tracker pose requires redraw");
    require(demand.needsRender(true, 20, 0, -1.0, false), "device removal requires redraw");
    require(demand.needsRender(true, 20, 1, 0.5, false), "GLB seek requires redraw");
    require(demand.needsRender(true, 20, 1, -1.0, true), "recording overlays remain animated");
    demand.presented(20, 1, -1.0, true);
    require(demand.needsRender(true, 20, 1, -1.0, false), "stopping recording must erase its overlay");
    demand.dirty = true;
    require(!demand.needsRender(false, 21, 1, 0.5, true), "hidden viewport must not render");
    require(demand.needsRender(true, 20, 1, -1.0, false), "restore retains pending redraw");
}

} // namespace ovtr::test
