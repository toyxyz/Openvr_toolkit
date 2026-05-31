#include "TestCases.h"
#include "TestSupport.h"

#include "platform/win32/AppState.h"
#include "platform/win32/PoseSamplingWorker.h"
#include "recording/RecordingController.h"
#include "recording/SamplingScheduler.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <system_error>

namespace ovtr::test {

void testWin32PoseSamplingWorker()
{
    auto ownedState = std::make_unique<ovtr::win32::AppWindowState>();
    ovtr::win32::AppWindowState& state = *ownedState;

    ovtr::PosePollResult poses;
    poses.timestampNs = 123;
    poses.poses = makeTestFrame(0).poses;
    ovtr::win32::storeLatestPoseSnapshot(state, poses);

    const ovtr::PosePollResult copied = ovtr::win32::copyLatestPoseSnapshot(state);
    require(copied.timestampNs == 123, "pose snapshot timestamp copied");
    require(copied.poses.size() == 1, "pose snapshot pose count copied");

    const std::filesystem::path testDir = std::filesystem::current_path() / ".tmp_ovtr_pose_sampling_worker";
    std::error_code ignored;
    std::filesystem::remove_all(testDir, ignored);

    const auto start = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        state.recordingStart = start;
        state.recordingScheduler = ovtr::SamplingScheduler(90.0);
        state.recordingScheduler.reset(start);

        ovtr::RecordingSession session;
        session.sessionId = "pose-worker-test";
        session.sessionName = "Pose Worker Test";
        require(
            state.recorder.start({testDir, session}),
            "pose worker recording start failed: " + state.recorder.lastError()
        );
    }

    require(ovtr::win32::appendRecordingFrameIfDue(state, poses, start), "first pose append succeeds");
    require(ovtr::win32::appendRecordingFrameIfDue(state, poses, start + std::chrono::milliseconds(1)), "early pose append skips");
    require(ovtr::win32::appendRecordingFrameIfDue(state, poses, start + std::chrono::milliseconds(12)), "second pose append succeeds");

    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        require(state.recorder.frameCount() == 2, "pose worker appends only due frames");
        require(state.recordingDroppedFrames == 0, "pose worker reports no drops for timely frames");
        require(state.recorder.stop(0.024, state.recordingDroppedFrames), "pose worker recording stop failed");
    }

    std::filesystem::remove_all(testDir, ignored);
}

} // namespace ovtr::test
