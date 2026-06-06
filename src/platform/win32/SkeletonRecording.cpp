#include "platform/win32/SkeletonRecording.h"

#include "platform/win32/AppRecordingState.h"
#include "platform/win32/AppProfileState.h"

#include <mutex>
#include <utility>

namespace ovtr::win32 {

void beginSkeletonRecording(SkeletonRecordingClip& clip, const MappingActor& actor)
{
    clip = SkeletonRecordingClip{};
    clip.active = true;
    clip.actorId = actor.id;
    clip.profile = actor.profile;
    clip.actorName = actor.profile.name;
}

void finishSkeletonRecording(SkeletonRecordingClip& clip) noexcept
{
    clip.active = false;
}

bool hasSkeletonRecordingFrames(const SkeletonRecordingClip& clip) noexcept
{
    return !clip.frames.empty();
}

std::size_t skeletonRecordingFrameCount(const SkeletonRecordingClip& clip) noexcept
{
    return clip.frames.size();
}

void recordSkeletonFrameIfRecording(
    AppRecordingState& recordingState,
    const MappingActor& actor,
    const std::chrono::steady_clock::time_point now
) {
    std::lock_guard<std::mutex> lock(recordingState.recordingMutex);
    SkeletonRecordingClip& clip = recordingState.skeletonRecording;
    if (recordingState.recorder.state() != ovtr::RecorderState::Recording ||
        !clip.active ||
        clip.actorId != actor.id ||
        !actor.liveJointsValid) {
        return;
    }

    SkeletonRecordingFrame frame;
    frame.timeSeconds = std::chrono::duration<double>(now - recordingState.recordingStart).count();
    frame.pose = actor.liveSkeletonPose;
    frame.pose.timeSeconds = frame.timeSeconds;
    clip.frames.push_back(std::move(frame));
}

} // namespace ovtr::win32
