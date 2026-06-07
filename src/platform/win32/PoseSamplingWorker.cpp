#include "platform/win32/PoseSamplingWorker.h"

#include "platform/win32/AppState.h"
#include "platform/win32/OriginState.h"
#include "platform/win32/VmcPoseBuilder.h"

#include <array>
#include <cstdint>
#include <exception>
#include <string>
#include <thread>
#include <utility>

namespace ovtr::win32 {
namespace {

using Clock = std::chrono::steady_clock;

Clock::duration poseSamplingInterval()
{
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / kPoseSamplingTargetFps)
    );
}

std::uint64_t nowNs()
{
    const auto now = Clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

void postStatusUpdate(HWND hwnd) noexcept
{
    if (hwnd != nullptr) {
        PostMessageW(hwnd, kPoseSamplingStatusMessage, 0, 0);
    }
}

void setPoseWorkerError(AppWindowState& state, const std::string& message, HWND hwnd)
{
    {
        std::lock_guard<std::mutex> lock(state.recordingMutex);
        state.recordingError = message;
    }
    postStatusUpdate(hwnd);
}

void runPoseSamplingWorker(HWND hwnd, AppWindowState& state)
{
    auto nextSample = Clock::now();
    const auto interval = poseSamplingInterval();

    while (!state.poseSamplingStopRequested.load(std::memory_order_acquire)) {
        const auto now = Clock::now();
        if (now >= nextSample) {
            if (!pollAndRecordPoseSampleOnce(state, now)) {
                postStatusUpdate(hwnd);
            }
            do {
                nextSample += interval;
            } while (nextSample <= now);
        }

        const auto remaining = nextSample - Clock::now();
        if (remaining > std::chrono::milliseconds(2)) {
            Sleep(1);
        } else {
            Sleep(0);
        }
    }
}

} // namespace

void startPoseSamplingWorker(HWND hwnd, AppWindowState& state)
{
    if (state.poseSamplingThread.joinable()) {
        return;
    }

    state.poseSamplingStopRequested.store(false, std::memory_order_release);
    try {
        state.poseSamplingThread = std::thread([hwnd, &state]() {
            try {
                runPoseSamplingWorker(hwnd, state);
            } catch (const std::exception& error) {
                setPoseWorkerError(state, "pose sampling worker failed: " + std::string(error.what()), hwnd);
            } catch (...) {
                setPoseWorkerError(state, "pose sampling worker failed: unknown error", hwnd);
            }
        });
    } catch (const std::exception& error) {
        setPoseWorkerError(state, "pose sampling worker start failed: " + std::string(error.what()), hwnd);
    }
}

void stopPoseSamplingWorker(AppWindowState& state) noexcept
{
    state.poseSamplingStopRequested.store(true, std::memory_order_release);
    if (state.poseSamplingThread.joinable()) {
        state.poseSamplingThread.join();
    }
}

ovtr::PosePollResult copyLatestPoseSnapshot(const AppWindowState& state)
{
    std::lock_guard<std::mutex> lock(state.poseMutex);
    return state.latestPoseSnapshot;
}

void storeLatestPoseSnapshot(AppWindowState& state, ovtr::PosePollResult poses)
{
    std::lock_guard<std::mutex> lock(state.poseMutex);
    state.latestPoseSnapshot = std::move(poses);
}

bool appendRecordingFrameIfDue(
    AppWindowState& state,
    const ovtr::PosePollResult& poses,
    const std::chrono::steady_clock::time_point now
)
{
    std::lock_guard<std::mutex> recordingLock(state.recordingMutex);
    if (state.recorder.state() != ovtr::RecorderState::Recording ||
        !state.recordingScheduler.shouldSample(now)) {
        return true;
    }

    const ovtr::SampleTiming timing = state.recordingScheduler.markSampled(now);
    state.recordingDroppedFrames += timing.droppedFrames;

    bool originEnabled = false;
    std::array<float, 3> originOffset{};
    std::array<float, 3> originRotationDegrees{};
    {
        std::lock_guard<std::mutex> originLock(state.originMutex);
        originEnabled = state.originEnabled;
        originOffset = state.originOffset;
        originRotationDegrees = state.originRotationDegrees;
    }

    ovtr::FrameSample frame;
    frame.frameIndex = timing.frameIndex;
    frame.timestampNs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - state.recordingStart).count()
    );
    frame.timeSeconds = static_cast<double>(frame.timestampNs) / 1'000'000'000.0;
    frame.poses = applyOriginToPoses(poses, originEnabled, originOffset, originRotationDegrees).poses;

    if (!state.recorder.appendFrame(frame)) {
        state.recordingError = state.recorder.lastError();
        return false;
    }
    return true;
}

bool pollAndRecordPoseSampleOnce(
    AppWindowState& state,
    const std::chrono::steady_clock::time_point now
)
{
    ovtr::PosePollResult poses;
    bool providerPosesAvailable = false;
    {
        std::lock_guard<std::mutex> providerLock(state.providerMutex);
        providerPosesAvailable = state.provider.isInitialized() && state.provider.pollPoses(poses);
    }
    if (!providerPosesAvailable) {
        poses.timestampNs = nowNs();
    }
    appendVmcFingerPoses(state, poses);
    if (!providerPosesAvailable && poses.poses.empty()) {
        storeLatestPoseSnapshot(state, poses);
        return true;
    }
    {
        std::lock_guard<std::mutex> smoothingLock(state.realtimeSmoothingMutex);
        state.realtimePoseSmoother.setPreset(state.realtimeSmoothingPreset);
        if (state.realtimeSmoothingEnabled) {
            state.realtimePoseSmoother.apply(poses);
        } else {
            state.realtimePoseSmoother.reset();
        }
    }
    storeLatestPoseSnapshot(state, poses);
    ++state.posePollFrames;
    return appendRecordingFrameIfDue(state, poses, now);
}

} // namespace ovtr::win32
