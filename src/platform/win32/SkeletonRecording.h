#pragma once

#include "platform/win32/SkeletonRecordingTypes.h"

#include <chrono>
#include <cstddef>

namespace ovtr::win32 {

struct AppRecordingState;
struct MappingActor;

void beginSkeletonRecording(SkeletonRecordingClip& clip, const MappingActor& actor);
void finishSkeletonRecording(SkeletonRecordingClip& clip) noexcept;
bool hasSkeletonRecordingFrames(const SkeletonRecordingClip& clip) noexcept;
std::size_t skeletonRecordingFrameCount(const SkeletonRecordingClip& clip) noexcept;
void recordSkeletonFrameIfRecording(
    AppRecordingState& recordingState,
    const MappingActor& actor,
    std::chrono::steady_clock::time_point now
);

} // namespace ovtr::win32
