#pragma once

#include "data/SessionTypes.h"
#include "recording/BinarySessionWriter.h"
#include "recording/RecordingState.h"
#include "recording/SessionManifest.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace ovtr {

struct RecordingStartOptions {
    std::filesystem::path sessionFolder;
    RecordingSession session;
};

class RecordingController {
public:
    bool start(const RecordingStartOptions& options);
    bool appendFrame(const FrameSample& frame);
    bool pause();
    bool resume();
    bool stop(double durationSeconds, std::uint64_t droppedFrames);
    void reset();

    RecorderState state() const;
    const RecordingSession& session() const;
    std::uint64_t frameCount() const;
    const std::string& lastError() const;

private:
    bool writeManifest(bool finalized, double durationSeconds, std::uint64_t droppedFrames);
    void setError(std::string message);

    RecorderState state_ = RecorderState::Idle;
    RecordingSession session_;
    std::filesystem::path sessionFolder_;
    BinarySessionWriter writer_;
    std::uint64_t frameCount_ = 0;
    std::string lastError_;
};

} // namespace ovtr
