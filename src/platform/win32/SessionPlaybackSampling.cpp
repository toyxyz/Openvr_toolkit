#include "platform/win32/SessionPlayback.h"
#include "platform/win32/PerformanceCapture.h"

#include "platform/win32/AppState.h"
#include "platform/win32/SessionPlaybackFrameIndex.h"
#include "platform/win32/VmcLegRotationContinuity.h"

#include <utility>

namespace ovtr::win32 {
namespace {

bool loadedSessionFrameDiscontinuous(const AppLoadedSessionState& state, const std::uint64_t frameIndex) noexcept
{
    if (!state.loadedSessionLastSampledFrameValid) {
        return true;
    }
    const std::uint64_t previous = state.loadedSessionLastSampledFrameIndex;
    if (frameIndex <= previous) {
        return frameIndex != previous;
    }
    return frameIndex > previous + 6;
}

} // namespace

bool sampleLoadedSessionFrame(AppWindowState& state)
{
    if (!state.loadedSessionActive) {
        return false;
    }
    const std::uint64_t frameIndex = loadedSessionFrameIndexForPlayback(state);
    // Sessions are immutable while loaded. Reuse a displayed frame until playback
    // advances; explicit seeks already invalidate this cache.
    if (state.loadedSessionLastSampledFrameValid && state.loadedSessionLastSampledFrameIndex == frameIndex) {
        return true;
    }
    if (loadedSessionFrameDiscontinuous(state, frameIndex)) {
        resetMappingActorsLiveContinuity(state);
        resetVmcLegRotationContinuity(state.vmcLegRotationContinuity);
    }
    FrameSample frame;
    PerformanceScope trace("read_frame");
    if (!state.loadedSessionReader.readFrame(frameIndex, frame)) {
        state.loadedSessionStatusMessage = state.loadedSessionReader.lastError();
        state.loadedSessionPlaying = false;
        state.playbackTimeline.playing = false;
        state.importedScenePlaying = false;
        return false;
    }
    state.poses.timestampNs = frame.timestampNs;
    state.poses.poses = std::move(frame.poses);
    state.loadedSessionLastSampledFrameIndex = frameIndex;
    state.loadedSessionLastSampledFrameValid = true;
    return true;
}

} // namespace ovtr::win32
